/*
 * GccKdm.hh
 *
 *  Created on: Jun 7, 2010
 *      Author: kgirard
 */

#ifndef GCCKDM_GCCKDMPLUGIN_HH_
#define GCCKDM_GCCKDMPLUGIN_HH_

#include "gcckdm/GccKdmConfig.hh"
#include "gcckdm/GccKdmUtilities.hh"
#include "gcckdm/utilities/Singleton.hh"
#include <string>
#include <set>

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

    void finishUnit(void * event_data, void * data);
    void finishType(void * event_data, void * data);

    GccKdmPlugin(){};
    ~GccKdmPlugin(){};

private:
    typedef std::multiset<tree, DeclComparator> DeclSet;

    GccKdmPlugin(GccKdmPlugin const &); //undefined
    GccKdmPlugin & operator=(GccKdmPlugin const &); //undefined

    void collect(tree ns);
    void collectNamespace(tree ns);

    void process();

    void printDecl(tree decl);
    void printNamespaceDecl(tree namespaceDecl);
    void printFunctionDecl(tree functionDecl);
    void printVarDecl(tree varDecl);
    void printTypeDecl(tree typeDecl);
    void printClassDecl(tree classDecl);

    //std::string getScopeString(tree decl);

    std::string mName;
    tree mAst;
    DeclSet mDeclSet;
};

} // namespace gcckdm

#endif /* GCCKDM_GCCKDMPLUGIN_HH_ */
