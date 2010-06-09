/*
 * GccKdmPlugin.cc
 *
 *  Created on: Jun 7, 2010
 *      Author: kgirard
 */

#include "gcckdm/GccKdmPlugin.hh"
#include "gcckdm/GccKdmUtilities.hh"

#include <iostream>
#include <sstream>

using std::string;
using std::cerr;
using std::endl;

/**
 * Have to define this to ensure that GCC is able to play nice with our plugin
 */
int plugin_is_GPL_compatible;

namespace
{
enum AccessSpec
{
  public_, protected_, private_
};

const char* AccessSpecStr[] =
{
  "public", "protected", "private"
};



extern "C" int plugin_init(struct plugin_name_args *plugin_info, struct plugin_gcc_version *version)
{
    int retValue(0);

    //Recommended version check
    if (plugin_default_version_check(version, &gcc_version))
    {
        gcckdm::GccKdmPlugin & kdmPlugin = gcckdm::GccKdmPlugin::Instance();
        kdmPlugin.name(plugin_info->base_name);

        // Process any plugin arguments
        // TODO

        // Register callbacks.
        //
        //register_callback (pluginName.c_str(), PLUGIN_PASS_MANAGER_SETUP, NULL, &pass_info);
        kdmPlugin.registerCallbacks();
    }
    else
    {
        retValue = 1;
    }

    return retValue;
}

extern "C" void executeGccKdm(void *event_data, void *data)
{
    gcckdm::GccKdmPlugin & kdmPlugin = gcckdm::GccKdmPlugin::Instance();
    kdmPlugin.generateKdm(event_data, data);
}

extern "C" void executePreGeneric(void *event_data, void *data)
{
    gcckdm::GccKdmPlugin & kdmPlugin = gcckdm::GccKdmPlugin::Instance();
    kdmPlugin.preGeneric(event_data, data);
}


std::string getScopeString(tree decl)
{
    std::string s, tmp;

    for (tree scope (CP_DECL_CONTEXT (decl));
         scope != global_namespace;
         scope = CP_DECL_CONTEXT (scope))
    {
      if (TREE_CODE (scope) == RECORD_TYPE)
      {
          scope= TYPE_NAME(scope);
      }
      tree id (DECL_NAME (scope));

      tmp = "::";
      tmp += (id != 0 ? IDENTIFIER_POINTER (id) : "<unnamed>");
      tmp += s;
      s.swap (tmp);
    }

    return s;
}


void printBasicDeclInfo(tree decl)
{
    int tc(TREE_CODE(decl));
    tree id(DECL_NAME (decl));
    string name(id ? IDENTIFIER_POINTER (id) : "<unnamed>");
    tree type (TREE_TYPE(decl));
    cerr << tree_code_name[tc] << " " << getScopeString(decl) << "::"<< name
         << " type " << tree_code_name[TREE_CODE(type)]
         << " at " <<  DECL_SOURCE_FILE (decl)  << ":" <<  DECL_SOURCE_LINE (decl) << endl;
}

}  // namespace


namespace gcckdm
{

GccKdmPlugin& GccKdmPlugin::Instance()
{
    static GccKdmPlugin instance;
    return instance;
}

std::string const & GccKdmPlugin::name() const
{
    return mName;
}

void GccKdmPlugin::name(std::string const & name)
{
    mName = name;
}

void GccKdmPlugin::registerCallbacks()
{
    register_callback(name().c_str(), PLUGIN_PRE_GENERICIZE, &executePreGeneric, 0);
    register_callback(name().c_str(), PLUGIN_EARLY_GIMPLE_PASSES_START, &executeGccKdm, 0);
    //register_callback(name().c_str(), PLUGIN_ALL_PASSES_START, &executeGccKdm, 0);
}

void GccKdmPlugin::preGeneric(void * event_data, void * data)
{
    std::string langName(lang_hooks.name);
    std::cerr << "GccKdmPlugin: Language " << langName << std::endl;

    //capture ast tree
    if (langName == "GNU C")
    {
        mAst = static_cast<tree> (event_data);
    }
    else if (langName == "GNU C++")
    {
        mAst = global_namespace;
    }

}

void GccKdmPlugin::generateKdm(void * event_data, void * data)
{
    collect(mAst);
    process();
}


void GccKdmPlugin::collect(tree t)
{
    if (TREE_CODE(t) == NAMESPACE_DECL)
    {
        collectNamespace(t);
    }
    else
    {
        if (!DECL_IS_BUILTIN(t))
        {
            mDeclSet.insert(t);
        }
    }
}

void GccKdmPlugin::collectNamespace(tree t)
{
    tree decl;
    cp_binding_level * level (NAMESPACE_LEVEL(t));

    //Collect declarations
    for (decl = level->names; decl; decl = TREE_CHAIN(decl))
    {
        if (DECL_IS_BUILTIN(decl))
        {
            continue;
        }
        mDeclSet.insert(decl);
    }

    //Traverse namespaces
    for (decl = level->namespaces; decl; decl = TREE_CHAIN(decl))
    {
        if (DECL_IS_BUILTIN(decl))
        {
            continue;
        }
        mDeclSet.insert(decl);
        collect(decl);
    }
}


void GccKdmPlugin::process()
{
    for (DeclSet::iterator i(mDeclSet.begin()), e(mDeclSet.end()); i != e; ++i)
    {
        printDecl(*i);
    }
}




void GccKdmPlugin::printDecl(tree decl)
{
    int treeCode(TREE_CODE(decl));

    switch (treeCode)
    {
        case NAMESPACE_DECL:
        {
            printNamespaceDecl(decl);
            break;
        }
        case FUNCTION_DECL:
        {
            printFunctionDecl(decl);
            break;
        }
        case TYPE_DECL:
        {
            printTypeDecl(decl);
            break;
        }
        case VAR_DECL:
        {
            printVarDecl(decl);
            break;
        }
        case CONST_DECL:
        {
            break;
        }
        case TEMPLATE_DECL:
        {
            break;
        }
        default:
        {
            std::cerr << "unsupported declaration " << tree_code_name[treeCode] << std::endl;
            break;
        }
    }
//
//
//    tree type (TREE_TYPE(decl));
//    int declCode (TREE_CODE (decl));
//    int tc;
//
//    if (type)
//    {
//        tc = TREE_CODE(type);
//        if (declCode == TYPE_DECL && tc == RECORD_TYPE)
//        {
//            // If DECL_ARTIFICIAL is true this is a class
//            // declaration. Otherwise this is a typedef.
//            //
//            if (DECL_ARTIFICIAL (decl))
//            {
//                //print class
//                return;
//            }
//        }
//    }
//
//
//    tree id (DECL_NAME (decl));
//    const char* name (id ? IDENTIFIER_POINTER (id) : "<unnamed>");
//
//
//    std::cerr << tree_code_name[tc] << " "
//         << namespaceString(decl) << "::" << name;
//
//    if (type)
//    {
//        std::cerr << " type " << tree_code_name[tc];
//    }
//
//    std::cerr   << " at " << DECL_SOURCE_FILE (decl) << ":"
//                << DECL_SOURCE_LINE (decl) << std::endl;
}


void GccKdmPlugin::printNamespaceDecl(tree namespaceDecl)
{
    printBasicDeclInfo(namespaceDecl);
//    int tc(TREE_CODE(namespaceDecl));
//    tree id(DECL_NAME (namespaceDecl));
//    std::string name(id ? IDENTIFIER_POINTER (id) : "<unnamed>");
//    tree type (TREE_TYPE(namespaceDecl));
//    std::cerr << tree_code_name[tc] << " " << getScopeString(namespaceDecl) << name;
//    std::cerr << " type " << tree_code_name[TREE_CODE(type)]
//              << " at " <<  DECL_SOURCE_FILE (namespaceDecl)  << ":" <<  DECL_SOURCE_LINE (namespaceDecl) << std::endl;
}



void GccKdmPlugin::printFunctionDecl(tree functionDecl)
{
//    int tc(TREE_CODE(functionDecl));
    tree id(DECL_NAME (functionDecl));
    std::string name(id ? IDENTIFIER_POINTER (id) : "<unnamed>");
//    tree type (TREE_TYPE(functionDecl));
//
//    std::string indent("    ");
//    std::cerr << tree_code_name[tc] << " " << getScopeString(functionDecl) << name;
//    std::cerr << " type " << tree_code_name[TREE_CODE(type)]
//              << " at " <<  DECL_SOURCE_FILE (functionDecl)  << ":" <<  DECL_SOURCE_LINE (functionDecl) << std::endl;

    printBasicDeclInfo(functionDecl);


    basic_block bb;

    if (gimple_has_body_p(functionDecl))
    {
        struct function *fn(DECL_STRUCT_FUNCTION(functionDecl));
        FOR_EACH_BB_FN(bb, fn)
        {
            for (gimple_stmt_iterator gsi = gsi_start_bb (bb); !gsi_end_p (gsi); gsi_next (&gsi))
            {
                gimple stmt = gsi_stmt (gsi);
                print_gimple_stmt(stderr, stmt, 0, 0);
            }
        }
    }
    else if (DECL_SAVED_TREE (functionDecl))
    {
        std::cerr << "function decl body: " << name << " DECL_SAVED_TREE"<< std::endl;
    }
    else
    {
        std::cerr << "\t" << "<function body empty>"<< std::endl;
    }

}

void GccKdmPlugin::printVarDecl(tree varDecl)
{
    printBasicDeclInfo(varDecl);
}

void GccKdmPlugin::printTypeDecl(tree typeDecl)
{
    tree type (TREE_TYPE(typeDecl));
    //int declCode(TREE_CODE(typeDecl));
    int treeCode(TREE_CODE(type));

    if (treeCode == RECORD_TYPE)
    {
        // if DECL_ARTIFICIAL is true this is a class
        // declaration.  Otherwise this is a typedef
        if (DECL_ARTIFICIAL(typeDecl))
        {
            printClassDecl(type);
        }
    }
    else
    {
        printBasicDeclInfo(typeDecl);
    }
}

void GccKdmPlugin::printClassDecl(tree type)
{
    type = TYPE_MAIN_VARIANT (type);
    tree decl (TYPE_NAME (type));
//    tree id (DECL_NAME (decl));
    printBasicDeclInfo(decl);

    // We are done if this is an incomplete
    // class declaration.
    //
    if (!COMPLETE_TYPE_P (type))
      return;


    // Traverse base information.
    //
    tree biv (TYPE_BINFO (type));
    size_t n (biv ? BINFO_N_BASE_BINFOS (biv) : 0);

    for (size_t i (0); i < n; i++)
    {
      tree bi (BINFO_BASE_BINFO (biv, i));

      // Get access specifier.
      //
      AccessSpec a (public_);

      if (BINFO_BASE_ACCESSES (biv))
      {
        tree ac (BINFO_BASE_ACCESS (biv, i));

        if (ac == 0 || ac == access_public_node)
          a = public_;
        else if (ac == access_protected_node)
          a = protected_;
        else
          a = private_;
      }

      bool virt (BINFO_VIRTUAL_P (bi));
      tree b_type (TYPE_MAIN_VARIANT (BINFO_TYPE (bi)));
      tree b_decl (TYPE_NAME (b_type));
      tree b_id (DECL_NAME (b_decl));
      const char* b_name (IDENTIFIER_POINTER (b_id));

      cerr << "\t" << AccessSpecStr[a] << (virt ? " virtual" : "")
           << " base " << getScopeString(b_decl) << "::" << b_name << endl;
    }


    // Traverse members.
    //
    DeclSet set;

    for (tree d (TYPE_FIELDS (type)); d != 0; d = TREE_CHAIN (d))
    {
      switch (TREE_CODE (d))
      {
      case TYPE_DECL:
        {
          if (!DECL_SELF_REFERENCE_P (d))
            set.insert (d);
          break;
        }
      case FIELD_DECL:
        {
          if (!DECL_ARTIFICIAL (d))
            set.insert (d);
          break;
        }
      default:
        {
          set.insert (d);
          break;
        }
      }
    }

    for (tree d (TYPE_METHODS (type)); d != 0; d = TREE_CHAIN (d))
    {
      if (!DECL_ARTIFICIAL (d))
        set.insert (d);
    }

    for (DeclSet::iterator i(set.begin()), e(set.end()); i != e; ++i)
    {
        printDecl(*i);
    }

}

//std::string GccKdmPlugin::getScopeString(tree decl)
//{
//    std::string s, tmp;
//
//    for (tree scope (CP_DECL_CONTEXT (decl));
//         scope != global_namespace;
//         scope = CP_DECL_CONTEXT (scope))
//    {
//      tree id (DECL_NAME (scope));
//
//      tmp = "::";
//      tmp += (id != 0 ? IDENTIFIER_POINTER (id) : "<unnamed>");
//      tmp += s;
//      s.swap (tmp);
//    }
//
//    return s;
//}

} // namespace gcckdm














//                std::cerr << indent << gimple_lineno(stmt) << ": " << gimple_code_name[gimple_code(stmt)] << std::endl;
//           for (gimple_stmt_iterator gsi = gsi_start_bb (bb); !gsi_end_p (gsi); gsi_next (&gsi))
//           {
//               walk_gimple_stmt(&gsi, gimpleStatmentCallBack, gimpleOperandCallBack, NULL);
//           }







//tree gimpleStatmentCallBack(gimple_stmt_iterator * gsi, bool * val, struct walk_stmt_info * info)
//{
//    std::cerr << "gimpleStatementCallBack" << std::endl;
//                    gimple stmt = gsi_stmt (*gsi);
//                    std::cerr << "    " << gimple_lineno(stmt) << ": " << gimple_code_name[gimple_code(stmt)] << std::endl;
//   return NULL_TREE;
//}
//
//tree gimpleOperandCallBack(tree * t, int * n, void * v)
//{
//    std::cerr << "gimpleOperandCallBack" << std::endl;
//    return NULL_TREE;
//}





//namespace
//{
//
//unsigned int ExecuteGccToKdm()
//{
//    warning (0, "%p", DECL_SAVED_TREE (current_function_decl));
//    return 0;
//}
//
//
////static struct gimple_opt_pass GccToKdmPass =
////{
////        {
////                GIMPLE_PASS,
////                "GccKdm",                     // name
////                NULL,                         // gate
////                ExecuteGccToKdm,              // execute
////                NULL,                         // sub
////                NULL,                         // next
////                0,                            // static_pass_number
////                TV_NONE,                      // tv_id
////                PROP_cfg,                     // properties_required
////                0,                            // properties_provided
////                0,                            // properties_destroyed
////                0,                            // todo_flags_start
////                TODO_dump_func                // todo_flags_finish
////        }
////};
//
//
////const std::string ArgRefPassName("ref-pass-name");
////const std::string ArgRefPassInstanceNum("ref-pass-instance-num");
//
//}


//    std::string pluginName(plugin_info->base_name);
//    int argc(plugin_info->argc);
//    struct plugin_argument* argv(plugin_info->argv);
//    std::string refPassName;
//    int refInstanceNumber(0);
//    const std::string RefPassName("ref-pass-name");
//    const std::string RefPassInstanceNum("ref-pass-instance-num");


//    //Process the plugin arguments.
//    // Supported arguments:
//    //  ref-pass-name=<PASS_NAME>
//    //  ref-pass-instance-num=<NUM>
//    for (int i = 0; i < argc; ++i)
//    {
//        if (ArgRefPassName == argv[i].key)
//        {
//            if (argv[i].value)
//            {
//                refPassName = argv[i].value;
//            }
//            else
//            {
//                warning(0, G_("option '-fplugin-arg-%s-ref-pass-name' requires a pass name"), pluginName.c_str());
//            }
//        }
//        else if (ArgRefPassInstanceNum == argv[i].key)
//        {
//            if (argv[i].value)
//            {
//                std::stringstream buff(argv[i].value);
//                buff >> refInstanceNumber;
//            }
//            else
//            {
//                warning (0, G_("option '-fplugin-arg-%s-ref-pass-instance-num' requires an integer value"), pluginName.c_str());
//            }
//        }
//    }
//
//    if (refPassName.empty())
//    {
//        error (G_("plugin %qs requires a reference pass name"), pluginName.c_str());
//        return 1;
//    }
//
//    struct register_pass_info pass_info;
//    pass_info.pass = &GccToKdmPass.pass;
//    pass_info.reference_pass_name = refPassName.c_str();
//    pass_info.ref_pass_instance_number = refInstanceNumber;
//    pass_info.pos_op = PASS_POS_INSERT_AFTER;


