/*
 * KdmTripleWriter.hh
 *
 *  Created on: Jun 21, 2010
 *      Author: kgirard
 */

#ifndef GCCKDM_KDMTRIPLEWRITER_KDMTRIPLEWRITER_HH_
#define GCCKDM_KDMTRIPLEWRITER_KDMTRIPLEWRITER_HH_

//#include <map>
//#include <set>
#include <queue>
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#include <iostream>
#include <boost/shared_ptr.hpp>

#include "gcckdm/GccKdmConfig.hh"
#include "gcckdm/GccKdmWriter.hh"
#include "gcckdm/GccKdmUtilities.hh"
#include "gcckdm/KdmPredicate.hh"
#include "gcckdm/KdmType.hh"
#include "gcckdm/kdmtriplewriter/PathHash.hh"

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
    typedef boost::filesystem::path Path;

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
    typedef std::tr1::unordered_map<tree, long> TreeMap;
    typedef std::tr1::unordered_map<Path, long> FileMap;
    typedef std::tr1::unordered_set<tree> TreeSet;
    typedef std::tr1::unordered_map<location_t, long> LocationMap;

    typedef std::queue<tree> TreeQueue;

    //    typedef std::map<tree, long> AstNodeReferenceMap;
//    typedef std::map<tree, long> TreeMap;
//    typedef std::set<tree> TreeSet;
//    typedef std::map<boost::filesystem::path, long> FileMap;

    long getSourceFileReferenceId(tree const decl);

    long getReferenceId(tree const node);
    long getSharedUnitReferenceId(tree const file);

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

    void processAstDeclarationNode(tree const decl);
    void processAstTypeNode(tree const decl);
    void processAstFunctionDeclarationNode(tree const functionDecl);
    void processAstFieldDeclarationNode(tree const fieldDecl);
    void processAstVariableDeclarationNode(tree const varDecl);


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
    void writeKdmSourceFile(Path const & file);
    void writeKdmCompilationUnit(Path const & file);
    void writeKdmCallableUnit(tree const functionDecl);
    long writeKdmReturnParameterUnit(tree const param);
    long writeKdmParameterUnit(tree const param);
    void writeKdmPrimitiveType(tree const type);
    void writeKdmPointerType(tree const type);
    void writeKdmRecordType(tree const type);
    void writeKdmSharedUnit(tree const file);
    long writeKdmItemUnit(tree const item);
    void writeKdmArrayType(tree const array);
    long writeKdmStorableUnit(tree const var);
    long writeKdmSignature(tree const function);
    long writeKdmSignatureDeclaration(tree const functionDecl);
    long writeKdmSignatureType(tree const functionType);
    long writeKdmSourceRef(long id,tree const var);
    long writeKdmSourceRef(long id, location_t const loc);

    long writeKdmActionElement(gimple const gs);
    std::string getUnaryRhsString(gimple const gs);
    std::string getBinaryRhsString(gimple const gs);
    std::string getTernaryRhsString(gimple const gs);
    long getBlockReferenceId(location_t const loc);

    void processGimpleSequence(tree const parent, gimple_seq const gs);
    void processGimpleStatement(tree const parent, gimple const gs);
    void processGimpleBindStatement(tree const parent, gimple const gs);
    void processGimpleAssignStatement(tree const parent, gimple const gs);

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

    TreeMap mReferencedNodes;
    TreeMap mReferencedSharedUnits;
    Path  mCompilationFile;
    FileMap mInventoryMap;
    LocationMap mBlockUnitMap;

    TreeSet mProcessedNodes;
    TreeQueue mNodeQueue;
};

} // namespace kdmtriplewriter

} // namespace gcckdm

#endif /* GCCKDM_KDMTRIPLEWRITER_KDMTRIPLEWRITER_HH_ */
