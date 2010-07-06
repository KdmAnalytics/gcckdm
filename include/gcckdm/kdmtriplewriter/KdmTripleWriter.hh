/*
 * KdmTripleWriter.hh
 *
 *  Created on: Jun 21, 2010
 *      Author: kgirard
 */

#ifndef GCCKDM_KDMTRIPLEWRITER_KDMTRIPLEWRITER_HH_
#define GCCKDM_KDMTRIPLEWRITER_KDMTRIPLEWRITER_HH_

#include <tr1/unordered_map>
#include <tr1/unordered_set>
#include <iostream>
#include <boost/shared_ptr.hpp>

#include "gcckdm/GccKdmConfig.hh"
#include "gcckdm/GccKdmWriter.hh"
#include "gcckdm/GccKdmUtilities.hh"
#include "gcckdm/KdmPredicate.hh"
#include "gcckdm/KdmType.hh"

namespace gcckdm
{

namespace kdmtriplewriter
{

/**
 * This class traverses the Gcc AST nodes passed to it and writes their KDM
 * representation to and output stream
 */
class KdmTripleWriter: public GccKdmWriter
{
public:
    typedef boost::shared_ptr<std::ostream> KdmSinkPtr;

    explicit KdmTripleWriter(KdmSinkPtr const & kdmSink);
    explicit KdmTripleWriter(boost::filesystem::path const & filename);
    virtual ~KdmTripleWriter();

    virtual void startTranslationUnit(boost::filesystem::path const & file);
    virtual void startKdmGimplePass();
    virtual void processAstNode(tree ast);
    virtual void finishKdmGimplePass();
    virtual void finishTranslationUnit();

    static const int KdmTripleVersion = 1;

private:
    typedef std::tr1::unordered_map<tree, long> AstNodeReferenceMap;
    typedef std::tr1::unordered_set<tree> TreeSet;

    long getSourceFileReferenceId(tree decl);

    long getReferenceId(tree node);
    long getSharedUnitReferenceId(tree file);

    enum
    {
        KdmElementId_Segment = 0,
        KdmElementId_CodeModel,
        KdmElementId_WorkbenchExtensionFamily,
        KdmElementId_HiddenStereoType,
        KdmElementId_CodeAssembly,
        KdmElementId_PrimitiveSharedUnit,
        KdmElementId_DerivedSharedUnit,
        KdmElementId_ClassSharedUnit,
        KdmElementId_InventoryModel,
        KdmElementId_CompilationUnit,
        KdmElementId_DefaultStart,
    };

    void processAstDeclarationNode(tree decl);
    void processAstTypeNode(tree decl);
    void processAstFunctionDeclarationNode(tree functionDecl);
    void processAstFieldDeclarationNode(tree fieldDecl);
    void processAstVariableDeclarationNode(tree varDecl);


    void writeVersionHeader();
    void writeDefaultKdmModelElements();

    /**
     * Write a SourceFile kdm element to the KdmSink stream using the given file
     *
     * Sample output:
     *
     * <10> <kdmtype> "source/SourceFile".
     * <10> <name> "test002.c".
     * <10> <path> "/tmp/c-tests/test002.c".
     * <10> <link::id> "/tmp/c-tests/test002.c".
     * <8> <contains> <10>.
     *
     * @param file the file to use to populate the SourceFile kdm element
     */
    void writeKdmSourceFile(boost::filesystem::path const & file);
    void writeKdmCompilationUnit(boost::filesystem::path const & file);
    void writeKdmCallableUnit(tree functionDecl);
    long writeKdmReturnParameterUnit(tree param);
    long writeKdmParameterUnit(tree param);
    void writeKdmPrimitiveType(tree type);
    void writeKdmPointerType(tree type);
    void writeKdmRecordType(tree type);
    void writeKdmSharedUnit(tree file);
    long writeKdmItemUnit(tree item);
    void writeKdmArrayType(tree array);
    long writeKdmStorableUnit(tree var);
    long writeKdmSignature(tree function);
    long writeKdmSignatureDeclaration(tree functionDecl);
    long writeKdmSignatureType(tree functionType);
    long writeKdmSourceRef(tree var);

    void processGimpleSequence(tree parent, gimple_seq gs);
    void processGimpleStatement(tree parent, gimple gs);
    void processGimpleBindStatement(tree parent, gimple gs);
    void processGimpleAssignStatement(tree parent, gimple gs);

    void writeTripleKdmType(long const subject, KdmType const & object);
    void writeTripleName(long const subject, std::string const & name);
    void writeTripleContains(long const parent, long const child);
    void writeTripleLinkId(long const subject, std::string const & name);

    /**
     *
     */
    void writeTriple(long const subject, KdmPredicate const & predicate, long const object);

    /**
     *
     */
    void writeTriple(long const subject, KdmPredicate const & predicate, KdmType const & object);

    /**
     *
     */
    void writeTriple(long const subject, KdmPredicate const & predicate, std::string const & object);


    KdmSinkPtr mKdmSink; /// Pointer to the kdm output stream
    long mKdmElementId;     /// The current element id, incremented for each new element

    AstNodeReferenceMap mReferencedNodes;
    AstNodeReferenceMap mReferencedSharedUnits;
    boost::filesystem::path  mCompilationFile;

    TreeSet mProcessedNodes;
};

} // namespace kdmtriplewriter

} // namespace gcckdm

#endif /* GCCKDM_KDMTRIPLEWRITER_KDMTRIPLEWRITER_HH_ */
