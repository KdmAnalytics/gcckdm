/*
 * GccKdm.hh
 *
 *  Created on: Jun 7, 2010
 *      Author: kgirard
 */

#ifndef GCCKDM_GCCKDMPLUGIN_HH_
#define GCCKDM_GCCKDMPLUGIN_HH_

#include <string>
#include <set>
#include "gcckdm/utilities/Singleton.hh"
#include "gcckdm/utilities/unique_ptr.hpp"

#include "gcckdm/GccKdmConfig.hh"
#include "gcckdm/GccKdmUtilities.hh"


/**
 * GCC Plugin Entry Point
 */
extern "C" int plugin_init(struct plugin_name_args *plugin_name_args, struct plugin_gcc_version *version);

/**
 * Wrapper function used in gcc call back to actually execute the KDM generation.
 * Calls the the GccKdmPlugin's generateKdm method
 *
 */
extern "C" void executeGccKdm(void *event_data, void *data);

/**
 * Wrapper function used in gcc call back to to get the AST at the PreGeneric
 * stage.  Calls the GccKdmPlugin's preGeneric method
 *
 */
extern "C" void executePreGeneric(void *event_data, void *data);


namespace gcckdm
{
    class GccKdmWriter;

/**
 * Singleton Class representing the GccKdmPlugin.
 *
 * The basic idea of this plugin is to first collection all declarations
 * in a translation unit, order in a set via location since GCC doesn't guarantee
 * the order of the declarations are in the order they are found in the
 * source and then iterate through the set generating KDM for each
 * different type of declaration.
 *
 */
class GccKdmPlugin : public Singleton<GccKdmPlugin>
{
public:
    /**
     * Returns the name of this plugin as a string
     *
     * @param return the name of this plugin as a string ie "libGccKdmPlugin.so"
     */
    std::string const & name() const;

    /**
     * Sets the name of this plugin to the given name
     */
    void name(std::string const & name);

    /**
     *  Register any GCC call back functions in this method
     */
    void registerCallbacks();

    /**
     * Generate KDM file
     */
    void generateKdm(void *event_data, void *data);

    /**
     * Retrieves the AST
     */
    void preGeneric(void * event_data, void * data);

    void startUnit(void * event_data, void * data);
    void finishUnit(void * event_data, void * data);
    void finishType(void * event_data, void * data);

    GccKdmPlugin(boost::unique_ptr<GccKdmWriter> writer);
    ~GccKdmPlugin(){};

private:
    typedef std::multiset<tree, DeclComparator> DeclSet;

    GccKdmPlugin(GccKdmPlugin const &); //undefined
    GccKdmPlugin & operator=(GccKdmPlugin const &); //undefined

    /**
     * Traverse the given AST and collect all declarations placing
     * them in a ordered set.  Send the ordered declarations to
     * the GccKdmWriter instance
     *
     * @param ast the Gcc AST to traverse, can be the global namespace or any AST node
     */
    void traverse(tree ast);


    void collectNamespaceDeclarations(tree ns);
    void collectDeclarations(tree decl);

    void processCollectedDeclarations();
    void processDeclaration(tree decl);
    void processNamespaceDeclaration(tree ns);
    void processFunctionDeclaration(tree funcDecl);
    void processType(tree type);
    void processRecordOrUnionType(tree type);
//    void collect(tree ns);
//    void collectNamespace(tree ns);
//
//    void process();

//    void printDecl(tree decl);
//    void printNamespaceDecl(tree namespaceDecl);
//    void printFunctionDecl(tree functionDecl);
//    void printVarDecl(tree varDecl);
//    void printTypeDecl(tree typeDecl);
//    void printClassDecl(tree classDecl);

    //std::string getScopeString(tree decl);

    std::string mName;
    tree mAst;
    DeclSet mDeclSet;
    boost::unique_ptr<GccKdmWriter> mWriter;
};

} // namespace gcckdm

#endif /* GCCKDM_GCCKDMPLUGIN_HH_ */
