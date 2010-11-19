//
// Copyright (c) 2010 KDM Analytics, Inc. All rights reserved.
// Date: Jun 7, 2010
// Author: Kyle Girard <kyle@kdmanalytics.com>, Ken Duck
//
// This file is part of libGccKdm.
//
// libGccKdm is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// libGccKdm is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with libGccKdm.  If not, see <http://www.gnu.org/licenses/>.
//

#include <iostream>
#include <boost/algorithm/string.hpp>
#include "gcckdm/utilities/null_ptr.hpp"
#include "gcckdm/utilities/unique_ptr.hpp"
#include "gcckdm/kdmtriplewriter/KdmTripleWriter.hh"
#include "gcckdm/utilities/null_deleter.hpp"
#include "boost/filesystem/operations.hpp"
#include "gcckdm/GccKdmVersion.hh"



/**
 * Have to define this to ensure that GCC is able to play nice with our plugin
 */
int plugin_is_GPL_compatible = 1;
namespace ktw = gcckdm::kdmtriplewriter;
namespace fs = boost::filesystem;
namespace
{

extern "C" int kdm_plugin_init(struct plugin_name_args *plugin_info, struct plugin_gcc_version *version);
extern "C" int plugin_init(struct plugin_name_args *plugin_info, struct plugin_gcc_version *version);
extern "C" void executeStartUnit(void *event_data, void *data);
extern "C" void executeFinishType(void *event_data, void *data);
#ifdef HAS_FINISH_DECL
extern "C" void executeFinishDecl(void *event_data, void *data);
#endif
extern "C" void executePreGeneric(void *event_data, void *data);
extern "C" unsigned int executeKdmGimplePass();
extern "C" void executeFinishUnit(void *event_data, void *data);

void registerCallbacks(char const * pluginName);
void processPluginArguments(struct plugin_name_args *plugin_info, ktw::KdmTripleWriter::KdmSinkPtr & kdmSink, ktw::KdmTripleWriter::Settings & settings);

boost::unique_ptr<gcckdm::GccAstListener> gccAstListener;

// Queue up tree object for latest processing (ie because gcc will fill in more info or loose track of them)
std::queue<tree> typeQueue;

struct opt_pass kdmGimplePass =
{ GIMPLE_PASS, // type
    "kdm", // name
    NULL, // gate
    executeKdmGimplePass, // execute
    NULL, // sub
    NULL, // next
    0, // static_pass_number
    TV_NONE, // tv_id
    PROP_gimple_any, // properties_required
    0, // properties_provided
    0, // properties_destroyed
    0, // todo_flags_start
    0 // todo_flags_finish
};

/**
 * This initialization function is used on windows for the "link" plugin.
 */
extern "C" int kdm_plugin_init(struct plugin_name_args *plugin_info, struct plugin_gcc_version *version)
{
  return plugin_init(plugin_info, version);
}

/**
 * This initialization function is used on linux for the standard DSO plugin
 *
 */
extern "C" int plugin_init(struct plugin_name_args *plugin_info, struct plugin_gcc_version *version)
{
  int retValue(0);

  //Recommended version check
  if (plugin_default_version_check(version, &gcc_version))
  {
    if (!std::string(gcc_version.basever).empty())
    {
      gcckdm::kdmtriplewriter::KdmTripleWriter::KdmSinkPtr kdmSink;
      gcckdm::kdmtriplewriter::KdmTripleWriter::Settings settings;
      processPluginArguments(plugin_info, kdmSink, settings);

      boost::unique_ptr<gcckdm::kdmtriplewriter::KdmTripleWriter> pWriter;
      //User specified non-default output
      if (kdmSink)
      {
        pWriter.reset(new gcckdm::kdmtriplewriter::KdmTripleWriter(kdmSink, settings));
      }
      // default output is file
      else
      {
        fs::path filename;
        if (settings.outputFile.empty())
        {
          filename = main_input_filename;
          filename.replace_extension(settings.outputExtension);
        }
        else
        {
          filename = settings.outputFile;
        }
        pWriter.reset(new gcckdm::kdmtriplewriter::KdmTripleWriter(filename, settings));
      }

      //Set out listener pointer
      gccAstListener.reset(pWriter.release());

      if (!settings.assemberOutput)
      {
        //Disable assembly output
        asm_file_name = HOST_BIT_BUCKET;
      }

      // Register callbacks.
      //
      registerCallbacks(plugin_info->base_name);
    }
  }
  else
  {
    retValue = 1;
  }

  return retValue;
}


void printHelpMessage(std::ostream & os)
{
  os  << "GccKdm Version " << gcckdm::GccKdmVersion
      << "\nOptions: \n"
      << "\n  --output=[LOC]                        Type of output: stderr, stdout, file (default: file)"
      << "\n  --output-dir=[DIR]                    Place all generated file in this directory"
      << "\n  --output-extension=[EXT]              Add the given suffix to generated output (default: .tkdm)"
      << "\n  --output-gimple=[true|false]          Include gimple in generated KDM (default: false)"
      << "\n  --output-complete-path=[true|false]   Attempt to complete all paths to source files"
      << "\n  --output-assembler=[true|false]       Generate assembler output (default: false)"
      << "\n  --bodies=[true|false]                 Generate MicroKDM for function bodies (default: true)"
      << "\n  --uids=[true|fasle]                   Generate UID's for Kdm Elements (default: true)"
      << "\n  --uid-graph=[true|false]              Generate UID graph in dot format (default: false)"
      << "\n  --debug-contains-check=[true|false]   Enable double containment checking (default: false)"
      << "\n  --help                                Prints this message"
      << "\n  --version                             Prints the GccKdm version"
      << std::endl;
}

void processPluginArguments(struct plugin_name_args *plugin_info, ktw::KdmTripleWriter::KdmSinkPtr & kdmSink, ktw::KdmTripleWriter::Settings & settings)
{
  // Process any plugin arguments
  int argc = plugin_info->argc;

  struct plugin_argument *argv = plugin_info->argv;

  for (int i = 0; i < argc; ++i)
  {
    std::string key(argv[i].key);
    if (key == "output")
    {
      std::string value(argv[i].value);
      if (value == "stdout")
      {
        kdmSink.reset(&std::cout, null_deleter());
      }
      else if (value == "stderr")
      {
        kdmSink.reset(&std::cout, null_deleter());
      }
      else if (value == "file")
      {
        //this is the default... handled below
      }
      else
      {
        warning(0, G_("plugin %qs: unrecognized argument %qs ignored"), plugin_info->base_name, value.c_str());
        continue;
      }
    }
    else if (key == "bodies")
    {
      std::string value(argv[i].value);
      if (value == "false")
      {
        settings.functionBodies = false;
      }
      else if (value.empty())
      {
        warning(0, G_("plugin %qs: unrecognized value for %qs ignored"), plugin_info->base_name, key.c_str());
        continue;
      }
    }
    else if (key == "uids")
    {
      std::string value(argv[i].value);
      if (value == "false")
      {
        settings.generateUids = false;
      }
    }
    else if (key == "uid-graph")
    {
      std::string value(argv[i].value);
      if (value == "true")
      {
        settings.generateUidGraph = true;
      }
    }
    else if (key == "debug-contains-check")
    {
      std::string value(argv[i].value);
      if (value == "true")
      {
        settings.containmentCheck = true;
      }
    }
    else if (key == "assembler-output")
    {
      settings.assemberOutput = true;
    }
    else if (key == "output-extension")
    {
      std::string value = argv[i].value;
      if (value.empty())
      {
        warning(0, G_("plugin %qs: unrecognized value for %qs ignored"), plugin_info->base_name, key.c_str());
        continue;
      }
      else
      {
        settings.outputExtension = value;
      }
    }
    else if (key == "output-gimple")
    {
      std::string value = argv[i].value;
      if (value == "true" )
      {
        settings.outputGimple = true;
      }
      else if (value == "false")
      {
        settings.outputGimple = false;
      }
      else
      {
        warning(0, G_("plugin %qs: unrecognized value for %qs ignored"), plugin_info->base_name, key.c_str());
      }
    }
    else if (key == "output-file")
    {
      std::string value = argv[i].value;
      if (value.empty())
      {
        warning(0, G_("plugin %qs: unrecognized value for %qs ignored"), plugin_info->base_name, key.c_str());
        continue;
      }
      else
      {
        settings.outputFile = value;
      }
    }
    else if (key == "output-dir")
    {
      std::string value(argv[i].value);
      fs::path outputDir(value);
      if (!fs::exists(outputDir))
      {
        fs::create_directory(outputDir);
      }
      settings.outputDir = outputDir;
    }
    else if (key == "output-complete-path")
    {
      std::string value(argv[i].value);
      if (value == "true")
      {
        settings.outputCompletePath = true;
      }
      else if (value == "false")
      {
        settings.outputCompletePath = false;
      }
      else
      {
        warning(0, G_("plugin %qs: unrecognized value for %qs ignored"), plugin_info->base_name, key.c_str());
        continue;
      }
    }
    else if (key == "help")
    {
      printHelpMessage(std::cout);
      exit(1);
    }
    else if (key == "version")
    {
      std::cout << "GccKdm Version: " << gcckdm::GccKdmVersion << std::endl;
      exit(1);
    }
    else
    {
      warning(0, G_("plugin %qs: unrecognized argument %qs ignored"), plugin_info->base_name, key.c_str());
    }
  }

}


/**
 * Convenience function to register all gcc plugin callback functions
 */
void registerCallbacks(char const * pluginName)
{
//  // Look for a pass introduced in GCC 4.5 prunes useful info(class members/etc) & disable it
//  // taken from dehyrda don't know if it effects us or not...
//#define DEFTIMEVAR(identifier__, name__)
//    identifier__,
//    enum
//    {
//      TV_NONE,
//#include "timevar.def"
//      TIMEVAR_LAST
//    };
//
//    struct opt_pass *p;
//    for(p = all_small_ipa_passes;p;p=p->next) {
//      if (p->tv_id != TV_IPA_FREE_LANG_DATA)
//        continue;
//      //disable it
//      p->execute = NULL;
//      break;
//    }



  //Called Once at the start of a translation unit
  register_callback(pluginName, PLUGIN_START_UNIT, static_cast<plugin_callback_func> (executeStartUnit), NULL);

  // In C we require notification of all types and functions so that we know to add them to the KDM. In
  // C++ we can just parse through the global namespace. The following callbacks are for C only.
  if(!gcckdm::isFrontendCxx())
  {
    // Called whenever a type has been parsed
    register_callback(pluginName, PLUGIN_FINISH_TYPE, static_cast<plugin_callback_func> (executeFinishType), NULL);
    // Called whenever a type has been parsed

#ifdef HAS_FINISH_DECL
    register_callback(pluginName, PLUGIN_FINISH_DECL, static_cast<plugin_callback_func> (executeFinishDecl), NULL);
#endif

  }
  //Attempt to get the very first gimple AST before any optimizations, called for every function
  struct register_pass_info pass_info;
  pass_info.pass = &kdmGimplePass;
  pass_info.reference_pass_name = all_lowering_passes->name;
  pass_info.ref_pass_instance_number = 0;
  pass_info.pos_op = PASS_POS_INSERT_AFTER;
  register_callback(pluginName, PLUGIN_PASS_MANAGER_SETUP, NULL, &pass_info);

  // Called when finished with the translation unit
  register_callback(pluginName, PLUGIN_FINISH_UNIT, static_cast<plugin_callback_func> (executeFinishUnit), NULL);
  //
  //
  //
  // Allows access to C/C++ ASTs... called for each function
  //        register_callback(pluginName, PLUGIN_PRE_GENERICIZE, static_cast<plugin_callback_func> (executePreGeneric), NULL);
  //
  //
  //    //    register_callback(name().c_str(), PLUGIN_ALL_IPA_PASSES_START, static_cast<plugin_callback_func> (executeGccKdm), NULL);
  //
  ////    register_callback(name().c_str(), PLUGIN_EARLY_GIMPLE_PASSES_START, static_cast<plugin_callback_func> (executeGccKdm), NULL);
  //
  //
  //    //    //Allows access to GIMPLE CFGs
  //    //    register_callback(name().c_str(), PLUGIN_ALL_PASSES_START, static_cast<plugin_callback_func> (executeGccKdm), NULL);
  //
  //
}

/**
 * Called once as GCC start parsing the the translation unit
 *
 */
extern "C" void executeStartUnit(void *event_data, void *data)
{
  if (!errorcount)
  {
    std::string filename(main_input_filename);
    boost::replace_all(filename, "\\","/");
    //We leave the path to the main_input_file unmodified... ie we don't
    //make it an absolute path to help support the case when compiled code
    //is preprocessed
    gccAstListener->startTranslationUnit(fs::path(filename));
  }
}


/**
 * Called after GCC finishes parsing a type
 */
extern "C" void executeFinishType(void *event_data, void *data)
{
  tree type = static_cast<tree> (event_data);
  if (!errorcount)
  {
    //Appending nodes to the queue instead of processing them immediately is
    //because gcc is overly lazy and does some things (like setting annonymous struct names)
    //sometime after completing the type
    // taken from dehyra_plugin.c
    typeQueue.push(type);
  }
}

#ifdef HAS_FINISH_DECL
/**
 * Called after GCC finishes parsing a declaration
 */
extern "C" void executeFinishDecl(void *event_data, void *data)
{
  tree decl = static_cast<tree> (event_data);
  int dc = TREE_CODE (decl);
  tree type = TREE_TYPE (decl);
  if (type)
  {
    int tc = TREE_CODE (type);
    if (dc == TYPE_DECL && tc == RECORD_TYPE)
    {
      if (!DECL_ARTIFICIAL (decl))
      {
        //we have a typedef
        gccAstListener->processAstNode(decl);
        gccAstListener->finishKdmGimplePass();
      }
    }
  }
}

#endif

/**
 * Called once for each function that is parsed by GCC
 */
extern "C" unsigned int executeKdmGimplePass()
{
  unsigned int retValue(0);

  if (!errorcount && !sorrycount)
  {
    gccAstListener->startKdmGimplePass();
    gccAstListener->processAstNode(current_function_decl);
    gccAstListener->finishKdmGimplePass();
  }
  return retValue;
}

/**
 *  Called once after GCC has finished processing the translation unit
 */
extern "C" void executeFinishUnit(void *event_data, void *data)
{
  // If we are processing C++, then start the processing from the global namespace, which
  // will provide access to all data in the system.
  if(gcckdm::isFrontendCxx())
  {
    gccAstListener->processAstNode(global_namespace);
  }

  if (!errorcount && !sorrycount)
  {
    //Process all recorded types
    for (; !typeQueue.empty(); typeQueue.pop())
    {
      //FIXME: It appears that GCC GC's some types before the end of the translation unit
      // Skip these for now and hope that we don't need them
      if (!TYPE_P (typeQueue.front()))
      {
        continue;
      }
      gccAstListener->processAstNode(typeQueue.front());
    }
    gccAstListener->finishTranslationUnit();
  }
}

} //namespace

