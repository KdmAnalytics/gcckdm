/*
 * KdmWriter.hh
 *
 *  Created on: Jun 21, 2010
 *      Author: kgirard
 */

#ifndef GCCKDM_KDMTRIPLEWRITER_KDMTRIPLEWRITER_HH_
#define GCCKDM_KDMTRIPLEWRITER_KDMTRIPLEWRITER_HH_

#include <iostream>
#include <queue>
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#include <boost/shared_ptr.hpp>
#include <gcckdm/utilities/unique_ptr.hpp>

#include "gcckdm/GccKdmConfig.hh"
#include "gcckdm/GccAstListener.hh"
#include "gcckdm/GccKdmUtilities.hh"
#include "gcckdm/KdmPredicate.hh"
#include "gcckdm/KdmType.hh"
#include "gcckdm/kdmtriplewriter/PathHash.hh"
#include "gcckdm/kdmtriplewriter/TripleWriter.hh"


#include "gcckdm/kdmtriplewriter/KdmTripleWriterFwd.hh"

namespace gcckdm
{

namespace kdmtriplewriter
{

/**
 * This class traverses the Gcc AST nodes passed to it and writes their KDM
 * representation to and output stream
 */
class KdmTripleWriter : public GccAstListener, public TripleWriter
{
public:
    static const int KdmTripleVersion = 1;

    typedef boost::shared_ptr<std::ostream> KdmSinkPtr;

    explicit KdmTripleWriter(KdmSinkPtr const & kdmSink);
    explicit KdmTripleWriter(Path const & filename);

    virtual ~KdmTripleWriter();

    virtual void startTranslationUnit(Path const & file);
    virtual void startKdmGimplePass();
    virtual void processAstNode(tree const ast);
    virtual void finishKdmGimplePass();
    virtual void finishTranslationUnit();

    /**
     *
     */
    virtual void writeTriple(long const subject, KdmPredicate const & predicate, long const object);

    /**
     *
     */
    virtual void writeTriple(long const subject, KdmPredicate const & predicate, KdmType const & object);

    /**
     *
     */
    virtual void writeTriple(long const subject, KdmPredicate const & predicate, std::string const & object);


    void writeTripleKdmType(long const subject, KdmType const & object);
    void writeTripleName(long const subject, std::string const & name);
    void writeTripleContains(long const parent, long const child);
    void writeTripleLinkId(long const subject, std::string const & name);


    long writeKdmSourceRef(long id, expanded_location const & xloc);

    long getReferenceId(tree const node);
    long getNextElementId();

private:
    typedef std::tr1::unordered_map<tree, long> TreeMap;
    typedef std::tr1::unordered_map<Path, long> FileMap;
    typedef std::tr1::unordered_set<tree> TreeSet;
    typedef std::queue<tree> TreeQueue;
    typedef boost::unique_ptr<GimpleKdmTripleWriter> GimpleWriter;

    long getSourceFileReferenceId(tree const decl);
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

    KdmSinkPtr mKdmSink; /// Pointer to the kdm output stream
    long mKdmElementId;     /// The current element id, incremented for each new element
    GimpleWriter mGimpleWriter;
    TreeMap mReferencedNodes;
    TreeMap mReferencedSharedUnits;
    Path  mCompilationFile;
    FileMap mInventoryMap;
    TreeSet mProcessedNodes;
    TreeQueue mNodeQueue;
};

} // namespace kdmtriplewriter

} // namespace gcckdm

#endif /* GCCKDM_KDMTRIPLEWRITER_KDMTRIPLEWRITER_HH_ */
