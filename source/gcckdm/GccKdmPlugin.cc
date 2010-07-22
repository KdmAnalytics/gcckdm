//
// Copyright (c) 2010 KDM Analytics, Inc. All rights reserved.
// Date: Jun 7, 2010
// Author: Kyle Girard <kyle@kdmanalytics.com>
//
// This file is part of libGccKdm.
//
// Foobar is free software: you can redistribute it and/or modify
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
#include "gcckdm/utilities/null_ptr.hpp"
#include "gcckdm/utilities/unique_ptr.hpp"
#include "gcckdm/kdmtriplewriter/KdmTripleWriter.hh"
#include "gcckdm/utilities/null_deleter.hpp"
#include "boost/filesystem/operations.hpp"

/**
 * Have to define this to ensure that GCC is able to play nice with our plugin
 */
int plugin_is_GPL_compatible = 1;

namespace
{
extern "C" int kdm_plugin_init(struct plugin_name_args *plugin_info, struct plugin_gcc_version *version);
extern "C" int plugin_init(struct plugin_name_args *plugin_info, struct plugin_gcc_version *version);
extern "C" void executeStartUnit(void *event_data, void *data);
extern "C" void executeFinishType(void *event_data, void *data);
extern "C" void executePreGeneric(void *event_data, void *data);
extern "C" unsigned int executeKdmGimplePass();
extern "C" void executeFinishUnit(void *event_data, void *data);

void registerCallbacks(char const * pluginName);

boost::unique_ptr<gcckdm::GccAstListener> gccAstListener;
// Queue up tree object for latest processing (ie because gcc will fill in more info or
// loose track of them)
VEC(tree,heap) *treeQueueVec = NULL;

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
 * Return true if we are parsing c++
 *
 * FIXME: This always returns true
 */
bool is_cpp()
{
  return true;
}

/** This initialization function is used on windows for the "link" plugin.
 *
 */
extern "C" int kdm_plugin_init(struct plugin_name_args *plugin_info, struct plugin_gcc_version *version)
{
  return plugin_init(plugin_info, version);
}

/** This initialization function is used on linux for the standard DSO plugin
 *
 */
extern "C" int plugin_init(struct plugin_name_args *plugin_info, struct plugin_gcc_version *version)
{
  int retValue(0);

  treeQueueVec = VEC_alloc(tree, heap, 10);

  //    struct opt_pass *p;
  //    for(p = all_small_ipa_passes;p;p=p->next) {
  //      if (p->tv_id != TV_IPA_FREE_LANG_DATA)
  //        continue;
  //      //disable it
  //      p->execute = NULL;
  //      break;
  //    }


  //Recommended version check
  if (plugin_default_version_check(version, &gcc_version))
  {
    if (!std::string(gcc_version.basever).empty())
    {
      // Process any plugin arguments
      int argc = plugin_info->argc;
      struct plugin_argument *argv = plugin_info->argv;

      for (int i = 0; i < argc; ++i)
      {
        std::string key(argv[i].key);
        if (key == "output")
        {
          gcckdm::kdmtriplewriter::KdmTripleWriter::KdmSinkPtr kdmSink;
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

          if (kdmSink)
          {
            gccAstListener.reset(new gcckdm::kdmtriplewriter::KdmTripleWriter(kdmSink));
          }
        }
        else
        {
          warning(0, G_("plugin %qs: unrecognized argument %qs ignored"), plugin_info->base_name, key.c_str());
        }
      }

      //default to file output
      if (!gccAstListener)
      {
        boost::filesystem::path filename(main_input_filename);
        filename.replace_extension(".tkdm");
        gccAstListener.reset(new gcckdm::kdmtriplewriter::KdmTripleWriter(filename));
      }

      //Disable assembly output
      asm_file_name = HOST_BIT_BUCKET;

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

void registerCallbacks(char const * pluginName)
{
  //    //Called at the start of a translation unit
  //    register_callback(pluginName, PLUGIN_START_UNIT, static_cast<plugin_callback_func> (executeStartUnit), NULL);

  if(!is_cpp())
  {
    // Called whenever a type has been parsed
    register_callback(pluginName, PLUGIN_FINISH_TYPE, static_cast<plugin_callback_func> (executeFinishType), NULL);

    //Attempt to get the very first gimple AST before any optimizations, called for every function
    struct register_pass_info pass_info;
    pass_info.pass = &kdmGimplePass;
    pass_info.reference_pass_name = all_lowering_passes->name;
    pass_info.ref_pass_instance_number = 0;
    pass_info.pos_op = PASS_POS_INSERT_AFTER;
    register_callback(pluginName, PLUGIN_PASS_MANAGER_SETUP, NULL, &pass_info);
  }

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

extern "C" void executeStartUnit(void *event_data, void *data)
{
}

extern "C" void executeAllPassStart(void *event_data, void *data)
{
}

/**
 * Function used for debug purposes.
 */
void print_decl (tree decl)
{
  int tc (TREE_CODE (decl));
  tree id (DECL_NAME (decl));
  const char* name (id
      ? IDENTIFIER_POINTER (id)
          : "<unnamed>");

  std::cerr << tree_code_name[tc] << " " << name << " at "
      << DECL_SOURCE_FILE (decl) << ":"
      << DECL_SOURCE_LINE (decl) << std::endl;
}

/**
 * Traverse the namespaces, pushing all found types onto the process queue
 */
void traverse_namespace (tree ns)
{
  tree decl;
  cp_binding_level* level (NAMESPACE_LEVEL (ns));

  // Traverse declarations.
  //
  for (decl = level->names;
      decl != 0;
      decl = TREE_CHAIN (decl))
  {
    if (DECL_IS_BUILTIN (decl))
      continue;

    if (!errorcount)
    {
      //Appending nodes to the queue instead of processing them immediately is
      //because gcc is overly lazy and does some things (like setting annonymous struct names)
      //sometime after completing the type
      // taken from dehyra_plugin.c
      VEC_safe_push(tree, heap, treeQueueVec, decl);
      //    print_decl(decl);
    }
  }

  // Traverse namespaces.
  //
  for(decl = level->namespaces;
      decl != 0;
      decl = TREE_CHAIN (decl))
  {
    if (DECL_IS_BUILTIN (decl))
      continue;

    if (!errorcount)
    {
      //Appending nodes to the queue instead of processing them immediately is
      //because gcc is overly lazy and does some things (like setting annonymous struct names)
      //sometime after completing the type
      // taken from dehyra_plugin.c
      VEC_safe_push(tree, heap, treeQueueVec, decl);
      //    print_decl(decl);
      traverse_namespace (decl);
    }
  }
}

extern "C" void executeFinishType(void *event_data, void *data)
{
  tree type(static_cast<tree> (event_data));
  //    if (!errorcount && TREE_CODE(type) == RECORD_TYPE)
  if (!errorcount)
  {
    //Appending nodes to the queue instead of processing them immediately is
    //because gcc is overly lazy and does some things (like setting annonymous struct names)
    //sometime after completing the type
    // taken from dehyra_plugin.c
    VEC_safe_push(tree, heap, treeQueueVec, type);
    //gccAstListener->processAstNode(static_cast<tree>(event_data));
  }
}

extern "C" void executePreGeneric(void *event_data, void *data)
{
  gccAstListener->processAstNode(static_cast<tree> (event_data));
}

extern "C" unsigned int executeKdmGimplePass()
{
  unsigned int retValue(0);

  if (!errorcount && !sorrycount)
  {
    boost::filesystem::path filename(main_input_filename);
    gccAstListener->startTranslationUnit(boost::filesystem::complete(filename));
    gccAstListener->startKdmGimplePass();
    gccAstListener->processAstNode(current_function_decl);
    gccAstListener->finishKdmGimplePass();
  }
  return retValue;
}

extern "C" void executeFinishUnit(void *event_data, void *data)
{
  if(is_cpp())
  {
    // Scan for all types defined in the global namespace, recursing through any other namespaces.
    traverse_namespace(global_namespace);
  }

  if (!errorcount && !sorrycount)
  {

    tree t;
    for (int i = 0; treeQueueVec && VEC_iterate (tree, treeQueueVec, i, t); ++i)
    {
      gccAstListener->processAstNode(t);
    }
    gccAstListener->finishTranslationUnit();
    VEC_free (tree, heap, treeQueueVec);
    treeQueueVec = nullptr;
  }
  int retValue(0);
  exit(retValue);
}
}

