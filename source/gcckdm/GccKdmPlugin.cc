/*
 * GccKdmPlugin.cc
 *
 *  Created on: Jun 7, 2010
 *      Author: kgirard
 */

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

extern "C" int plugin_init(struct plugin_name_args *plugin_info, struct plugin_gcc_version *version);
extern "C" void executeStartUnit(void *event_data, void *data);
extern "C" void executeFinishType(void *event_data, void *data);
extern "C" void executePreGeneric(void *event_data, void *data);
extern "C" unsigned int executeKdmGimplePass();
extern "C" void executeFinishUnit(void *event_data, void *data);

void registerCallbacks(char const * pluginName);

boost::unique_ptr<gcckdm::GccKdmWriter> kdmWriter;
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
                    kdmWriter.reset(new gcckdm::kdmtriplewriter::KdmTripleWriter(kdmSink));
                }
            }
            else
            {
                warning(0, G_("plugin %qs: unrecognized argument %qs ignored"), plugin_info->base_name, key.c_str());
            }
        }

        //default to file output
        if (!kdmWriter)
        {
            boost::filesystem::path filename(main_input_filename);
            filename.replace_extension(".tkdm");
            kdmWriter.reset(new gcckdm::kdmtriplewriter::KdmTripleWriter(filename));
        }

        //Disable assembly output
        asm_file_name = HOST_BIT_BUCKET;

        // Register callbacks.
        //
        registerCallbacks(plugin_info->base_name);
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

    // Called whenever a type has been parsed
    register_callback(pluginName, PLUGIN_FINISH_TYPE, static_cast<plugin_callback_func> (executeFinishType), NULL);

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

extern "C" void executeStartUnit(void *event_data, void *data)
{
}

extern "C" void executeAllPassStart(void *event_data, void *data)
{
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
        //kdmWriter->processAstNode(static_cast<tree>(event_data));
    }
}

extern "C" void executePreGeneric(void *event_data, void *data)
{
    kdmWriter->processAstNode(static_cast<tree>(event_data));
}

extern "C" unsigned int executeKdmGimplePass()
{
    unsigned int retValue(0);

    if (!errorcount && !sorrycount)
    {
        //    std::cerr << "======================start executeKdmGimplePass========================" << std::endl;
        boost::filesystem::path filename(main_input_filename);
        kdmWriter->startTranslationUnit(boost::filesystem::complete(filename));

        kdmWriter->startKdmGimplePass();
        //    if (global_namespace)
        //    {
        //        kdmWriter->processAstNode(global_namespace);
        //    }
        //    else
        //    {
        //        int count(0);
        //        struct cgraph_node *n;
        //        for (n = cgraph_nodes; n; n = n->next)
        //        {
        //            std::cerr << "executeKdmGimplePass: AST NodeCount: " << ++count << std::endl;
        //            kdmWriter->processAstNode(n->decl);
        //        }
        //    }
        kdmWriter->processAstNode(current_function_decl);
        kdmWriter->finishKdmGimplePass();
        //    std::cerr << "======================end executeKdmGimplePass========================" << std::endl;
    }
    return retValue;
}

extern "C" void executeFinishUnit(void *event_data, void *data)
{
    if (!errorcount && !sorrycount)
    {
        tree t;
        for (int i = 0; treeQueueVec && VEC_iterate (tree, treeQueueVec, i, t); ++i)
        {
            kdmWriter->processAstNode(t);
        }
        kdmWriter->finishTranslationUnit();
        VEC_free (tree, heap, treeQueueVec);
        treeQueueVec = nullptr;
    }
    int retValue(0);
    exit(retValue);
}

//
//std::string getScopeString(tree decl)
//{
//    std::string s, tmp;
//
//    for (tree scope(CP_DECL_CONTEXT (decl)); scope != global_namespace; scope = CP_DECL_CONTEXT (scope))
//    {
//        if (TREE_CODE (scope) == RECORD_TYPE)
//        {
//            scope = TYPE_NAME(scope);
//        }
//        tree id(DECL_NAME (scope));
//
//        tmp = "::";
//        tmp += (id != 0 ? IDENTIFIER_POINTER (id) : "<unnamed>");
//        tmp += s;
//        s.swap(tmp);
//    }
//
//    return s;
//}
//
//void printBasicDeclInfo(tree decl)
//{
//    int tc(TREE_CODE(decl));
//    tree id(DECL_NAME (decl));
//    string name(id ? IDENTIFIER_POINTER (id) : "<unnamed>");
//    tree type(TREE_TYPE(decl));
//    cerr << tree_code_name[tc] << " " << getScopeString(decl) << "::" << name << " type " << tree_code_name[TREE_CODE(type)] << " at "
//            << DECL_SOURCE_FILE (decl) << ":" << DECL_SOURCE_LINE (decl) << endl;
//}

} // namespace


//namespace gcckdm
//{
//
//GccKdmPlugin::GccKdmPlugin(boost::unique_ptr<GccKdmWriter> writer) :
//    mWriter(boost::move(writer))
//{
//
//}
//
//std::string const & GccKdmPlugin::name() const
//{
//    return mName;
//}
//
//void GccKdmPlugin::name(std::string const & name)
//{
//    mName = name;
//}
//
//void GccKdmPlugin::registerCallbacks()
//{
//    //Called at the start of a translation unit
//    register_callback(name().c_str(), PLUGIN_START_UNIT, static_cast<plugin_callback_func> (executeStartUnit), NULL);
//
////    //    // Called whenever a type has been parsed
////    register_callback(name().c_str(), PLUGIN_FINISH_TYPE, static_cast<plugin_callback_func> (executeFinishType), NULL);
//
//    //
//    //
//    //
//    //Allows access to C/C++ ASTs... called for each function
////    register_callback(name().c_str(), PLUGIN_PRE_GENERICIZE, static_cast<plugin_callback_func> (executePreGeneric), NULL);
//    //
//    //Attempt to get the very first gimple AST before any optimizations
//    struct register_pass_info pass_info;
//    pass_info.pass = &kdmPass;
//    pass_info.reference_pass_name = all_lowering_passes->name;
//    pass_info.ref_pass_instance_number = 0;
//    pass_info.pos_op = PASS_POS_INSERT_AFTER;
//    register_callback(name().c_str(), PLUGIN_PASS_MANAGER_SETUP, NULL, &pass_info);
//    //
//    //    //    register_callback(name().c_str(), PLUGIN_ALL_IPA_PASSES_START, static_cast<plugin_callback_func> (executeGccKdm), NULL);
//    //
//    ////    register_callback(name().c_str(), PLUGIN_EARLY_GIMPLE_PASSES_START, static_cast<plugin_callback_func> (executeGccKdm), NULL);
//    //
//    //
//    //    //    //Allows access to GIMPLE CFGs
//    //    //    register_callback(name().c_str(), PLUGIN_ALL_PASSES_START, static_cast<plugin_callback_func> (executeGccKdm), NULL);
//    //
//    //
//
//    // Called when finished with the translation unit
//    register_callback(name().c_str(), PLUGIN_FINISH_UNIT, static_cast<plugin_callback_func> (executeFinishUnit), NULL);
//
//}
//
//void GccKdmPlugin::preGeneric(void * event_data, void * data)
//{
//    cerr << endl << "=========PRE GENERIC START==========" << endl;
//    // Note:: preGeneric seems to be called for every function....
//
//    tree t = static_cast<tree> (event_data);
//    if (errorcount || DECL_CLONED_FUNCTION_P (t) || DECL_ARTIFICIAL(t))
//    {
//        return;
//    }
//
//    if (event_data)
//    {
//        traverse((tree)event_data);
////        collect((tree) event_data);
////        process();
//        mDeclSet.clear();
//    }
//    cerr << endl << "=========PRE GENERIC STOP==========" << endl;
//
//}
//
//void GccKdmPlugin::generateKdm(void * event_data, void * data)
//{
//    cerr << endl << "=========GENERATE KDM START==========" << endl;
//
//    if (event_data)
//    {
//        tree ast = static_cast<tree>(event_data);
//        traverse(ast);
//    }
//
////    if (event_data)
////    {
////        traverse((tree)event_data);
//////        collect((tree) event_data);
//////        process();
////        mDeclSet.clear();
////    }
////    else if (global_namespace)
////    {
////        traverse(global_namespace);
//////        collect(global_namespace);
//////        process();
////        mDeclSet.clear();
////    }
////    else
////    {
////        struct cgraph_node *n;
////        for (n = cgraph_nodes; n; n = n->next)
////        {
////            collectDeclarations(n->decl);
//////            collect(n->decl);
////        }
//////        process();
////        processCollectedDeclarations();
////        mDeclSet.clear();
////    }
//    cerr << endl << "=========GENERATE KDM STOP==========" << endl;
//}
//
//void GccKdmPlugin::startUnit(void * event_data, void * data)
//{
//    boost::filesystem::path filename(main_input_filename);
//    mWriter->start(boost::filesystem::complete(filename));
//}
//
//void GccKdmPlugin::finishUnit(void * event_data, void * data)
//{
//    mWriter->finish();
//    //
//    //    cerr << endl << "=========FINISH UNIT START==========" << endl;
//    //
//    //    if (global_namespace)
//    //    {
//    //        collect(global_namespace);
//    //    }
//    //    else
//    //    {
//    //        struct cgraph_node *n;
//    //        for (n = cgraph_nodes; n; n = n->next)
//    //        {
//    //            collect(n->decl);
//    //        }
//    //    }
//    //    process();
//    //    cerr << endl << "=========FINISH UNIT STOP ==========" << endl;
//}
//
//void GccKdmPlugin::finishType(void * event_data, void * data)
//{
//    cerr << endl << "=========FINISH TYPE START==========" << endl;
//    processType((tree)event_data);
//    //printClassDecl((tree)event_data);
////    if (event_data)
////    {
////        printDecl((tree) event_data);
////    }
//
//    cerr << endl << "=========FINISH TYPE STOP ==========" << endl;
//}
//
//
//void GccKdmPlugin::traverse(tree ast)
//{
//    collectDeclarations(ast);
//    processCollectedDeclarations();
//}
//
//void GccKdmPlugin::collectDeclarations(tree decl)
//{
//    if (TREE_CODE(decl) == NAMESPACE_DECL)
//    {
//        collectNamespaceDeclarations(decl);
//    }
//    else
//    {
//        if (!DECL_IS_BUILTIN(decl))
//        {
//            mDeclSet.insert(decl);
//        }
//    }
//}
//
//void GccKdmPlugin::collectNamespaceDeclarations(tree ns)
//{
//    tree decl;
//    cp_binding_level * level(NAMESPACE_LEVEL(ns));
//
//    //Collect declarations
//    for (decl = level->names; decl; decl = TREE_CHAIN(decl))
//    {
//        if (DECL_IS_BUILTIN(decl))
//        {
//            continue;
//        }
//        mDeclSet.insert(decl);
//    }
//
//    //Traverse namespaces
//    for (decl = level->namespaces; decl; decl = TREE_CHAIN(decl))
//    {
//        if (DECL_IS_BUILTIN(decl))
//        {
//            continue;
//        }
//        mDeclSet.insert(decl);
//        collectDeclarations(decl);
//    }
//}
//
//
//void GccKdmPlugin::processCollectedDeclarations()
//{
//    for (DeclSet::iterator i(mDeclSet.begin()), e(mDeclSet.end()); i != e; ++i)
//    {
//        processDeclaration(*i);
//    }
//}
//
//
//void GccKdmPlugin::processDeclaration(tree declaration)
//{
//    assert(DECL_P(declaration));
//
//    tree type(TREE_TYPE(declaration));
//    int declCode(TREE_CODE(declaration));
//
//    switch (declCode)
//    {
//        case NAMESPACE_DECL:
//        {
//            processNamespaceDeclaration(declaration);
//            break;
//        }
//        case TEMPLATE_DECL:
//        {
//            std::cerr << "unsupported declaration " << tree_code_name[declCode] << std::endl;
//            //TODO
//            break;
//        }
//        case FUNCTION_DECL:
//        {
//            processFunctionDeclaration(declaration);
//            break;
//        }
//        case FIELD_DECL:
//        {
////            processFieldDeclaration(declaration);
//            break;
//        }
//        case VAR_DECL:
//        {
////            processVariableDeclaration(declaration);
//            break;
//        }
//        case TYPE_DECL:
//        {
//            processType(type);
//            break;
//        }
//        case USING_DECL:
//        {
//            //Skip on Purpose
//            return;
//        }
//        default:
//        {
//            std::cerr << "unsupported declaration " << tree_code_name[declCode] << std::endl;
//
//        }
//    }
//}
//
//void GccKdmPlugin::processNamespaceDeclaration(tree ns)
//{
//
//}
//
//
//void GccKdmPlugin::processFunctionDeclaration(tree funcDecl)
//{
//    mWriter->writeCallableUnit(funcDecl);
//}
//
//void GccKdmPlugin::processType(tree type)
//{
////    std::string tag;
////    if (TREE_CODE(type) == RECORD_TYPE)
////    {
////        if (CLASSTYPE_DECLARED_CLASS (type))
////        {
////            tag = "Class";
////        }
////        else
////        {
////            tag = "Struct";
////        }
////    }
////    else
////    {
////        tag = "Union";
////    }
//
////    std::cerr << tag << std::endl;
//
////    tree t = TYPE_MAIN_VARIANT (type);
////    switch(TREE_CODE(t))
////    {
////        case UNION_TYPE:
////        case QUAL_UNION_TYPE:
////        case RECORD_TYPE:
////        {
////            tree id(DECL_NAME (t));
////            std::string name(id ? IDENTIFIER_POINTER (id) : "<unnamed>");
////
////            std::cerr << "RecordType" << name << endl;
////            break;
////        }
////        case COMPLEX_TYPE:
////        {
////            std::cerr << "ComplexType" << endl;
////            break;
////        }
////        default:
////        {
////            break;
////        }
////    }
//
//    //    //process original type first... see dehyra
////    tree typeDecl(TYPE_NAME(type));
////    if (typeDecl && TREE_CODE(typeDecl) == TYPE_DECL)
////    {
////        tree originalType = DECL_ORIGINAL_TYPE(typeDecl);
////        if (originalType)
////        {
////            processType(originalType);
////        }
////    }
////
////    switch(TREE_CODE(type))
////    {
////        case RECORD_TYPE:
////            //Fall Through
////        case UNION_TYPE:
////        {
////            processRecordOrUnionType(type);
////            break;
////        }
////        case ENUMERAL_TYPE:
////        {
////            //TODO
////            std::cerr << "unsupported enumeral type " << tree_code_name[TREE_CODE(type)] << std::endl;
////            break;
////        }
////        default:
////        {
////            std::cerr << "unsupported type " << tree_code_name[TREE_CODE(type)] << std::endl;
////            break;
////        }
////    }
//}
//
//void GccKdmPlugin::processRecordOrUnionType(tree c)
//{
//    //TODO
//    std::cerr << "Unsupported Record or Union Type" << std::endl;
//
//}
//
////void GccKdmPlugin::printDecl(tree decl)
////{
////    tree type(TREE_TYPE(decl));
////    if (type)
////    {
////        int declCode(TREE_CODE(decl));
////
////        switch (declCode)
////        {
////            case NAMESPACE_DECL:
////            {
////                printNamespaceDecl(decl);
////                break;
////            }
////            case FUNCTION_DECL:
////            {
////                printFunctionDecl(decl);
////                break;
////            }
////            case TYPE_DECL:
////            {
////                printTypeDecl(decl);
////                break;
////            }
////            case VAR_DECL:
////            {
////                printVarDecl(decl);
////                break;
////            }
////            case CONST_DECL:
////            {
////                break;
////            }
////            case TEMPLATE_DECL:
////            {
////                break;
////            }
////            default:
////            {
////                std::cerr << "unsupported declaration " << tree_code_name[treeCode] << std::endl;
////                break;
////            }
////        }
////    }
////}
////
////void GccKdmPlugin::printNamespaceDecl(tree namespaceDecl)
////{
////    printBasicDeclInfo(namespaceDecl);
////}
////
////void GccKdmPlugin::printFunctionDecl(tree functionDecl)
////{
////    mWriter->writeCallableUnit(functionDecl);
////    //    if (errorcount
////    //        || DECL_CLONED_FUNCTION_P (functionDecl)
////    //        || DECL_ARTIFICIAL(functionDecl)) return;
////    //
////    //
////    //    tree id(DECL_NAME (functionDecl));
////    //    std::string name(id ? IDENTIFIER_POINTER (id) : "<unnamed>");
////    //
////    //    printBasicDeclInfo(functionDecl);
////    //
////    //
////    //    if (gimple_has_body_p(functionDecl))
////    //    {
////    //        if (!gimple_body(functionDecl))
////    //        {
////    //            basic_block bb;
////    //            struct function *fn(DECL_STRUCT_FUNCTION(functionDecl));
////    //            FOR_EACH_BB_FN(bb, fn)
////    //            {
////    //                cerr << "  basic_block " << endl;
////    //                print_gimple_seq(stderr, bb_seq(bb), 2, 0);
////    //            }
////    //        }
////    //        else
////    //        {
////    //            gimple_seq seq = gimple_body(functionDecl);
////    //            print_gimple_seq(stderr, seq, 0, 0);
////    //        }
////    //    }
////    //    else if (DECL_SAVED_TREE (functionDecl))
////    //    {
////    //        std::cerr << "function decl body: " << name << " DECL_SAVED_TREE" << std::endl;
////    //    }
////    //    else
////    //    {
////    //        std::cerr << "\t" << "<function body empty>" << std::endl;
////    //    }
////
////}
////
////void GccKdmPlugin::printVarDecl(tree varDecl)
////{
////    printBasicDeclInfo(varDecl);
////}
////
////void GccKdmPlugin::printTypeDecl(tree typeDecl)
////{
////    tree type(TREE_TYPE(typeDecl));
////    //int declCode(TREE_CODE(typeDecl));
////    int treeCode(TREE_CODE(type));
////
////    if (treeCode == RECORD_TYPE)
////    {
////        // if DECL_ARTIFICIAL is true this is a class
////        // declaration.  Otherwise this is a typedef
////        if (DECL_ARTIFICIAL(typeDecl))
////        {
////            printClassDecl(type);
////        }
////    }
////    else
////    {
////        printBasicDeclInfo(typeDecl);
////    }
////}
////
////void GccKdmPlugin::printClassDecl(tree type)
////{
////    type = TYPE_MAIN_VARIANT (type);
////    tree decl(TYPE_NAME (type));
////    //    tree id (DECL_NAME (decl));
////    printBasicDeclInfo(decl);
////
////    // We are done if this is an incomplete
////    // class declaration.
////    //
////    if (!COMPLETE_TYPE_P (type))
////        return;
////
////    // Traverse base information.
////    //
////    tree biv(TYPE_BINFO (type));
////    size_t n(biv ? BINFO_N_BASE_BINFOS (biv) : 0);
////
////    for (size_t i(0); i < n; i++)
////    {
////        tree bi(BINFO_BASE_BINFO (biv, i));
////
////        // Get access specifier.
////        //
////        AccessSpec a(public_);
////
////        if (BINFO_BASE_ACCESSES (biv))
////        {
////            tree ac(BINFO_BASE_ACCESS (biv, i));
////
////            if (ac == 0 || ac == access_public_node)
////                a = public_;
////            else if (ac == access_protected_node)
////                a = protected_;
////            else
////                a = private_;
////        }
////
////        bool virt(BINFO_VIRTUAL_P (bi));
////        tree b_type(TYPE_MAIN_VARIANT (BINFO_TYPE (bi)));
////        tree b_decl(TYPE_NAME (b_type));
////        tree b_id(DECL_NAME (b_decl));
////        const char* b_name(IDENTIFIER_POINTER (b_id));
////
////        cerr << "\t" << AccessSpecStr[a] << (virt ? " virtual" : "") << " base " << getScopeString(b_decl) << "::" << b_name << endl;
////    }
////
////    // Traverse members.
////    //
////    DeclSet set;
////
////    for (tree d(TYPE_FIELDS (type)); d != 0; d = TREE_CHAIN (d))
////    {
////        switch (TREE_CODE (d))
////        {
////            case TYPE_DECL:
////            {
////                if (!DECL_SELF_REFERENCE_P (d))
////                    set.insert(d);
////                break;
////            }
////            case FIELD_DECL:
////            {
////                if (!DECL_ARTIFICIAL (d))
////                    set.insert(d);
////                break;
////            }
////            default:
////            {
////                set.insert(d);
////                break;
////            }
////        }
////    }
////
////    for (tree d(TYPE_METHODS (type)); d != 0; d = TREE_CHAIN (d))
////    {
////        if (!DECL_ARTIFICIAL (d))
////            set.insert(d);
////    }
////
////    for (DeclSet::iterator i(set.begin()), e(set.end()); i != e; ++i)
////    {
////        printDecl(*i);
////    }
////
////}
//
//} // namespace gcckdm

//enum AccessSpec
//{
//    public_, protected_, private_
//};
//
//const char* AccessSpecStr[] =
//{ "public", "protected", "private" };
//


//unsigned int executeKdmPass()
//{
//    gcckdm::GccKdmPlugin & kdmPlugin = gcckdm::GccKdmPlugin::Instance();
//    kdmPlugin.generateKdm(current_function_decl, NULL);
//    return 0;
//}

