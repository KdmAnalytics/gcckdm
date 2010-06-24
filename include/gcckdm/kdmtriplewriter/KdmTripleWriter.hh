/*
 * KdmTripleWriter.hh
 *
 *  Created on: Jun 21, 2010
 *      Author: kgirard
 */

#ifndef GCCKDM_KDMTRIPLEWRITER_KDMTRIPLEWRITER_HH_
#define GCCKDM_KDMTRIPLEWRITER_KDMTRIPLEWRITER_HH_

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

class KdmTripleWriter: public GccKdmWriter
{
public:
    typedef boost::shared_ptr<std::ostream> KdmSinkPtr;

    explicit KdmTripleWriter(KdmSinkPtr const & kdmSink);
    explicit KdmTripleWriter(boost::filesystem::path const & filename);

    virtual void start(boost::filesystem::path const & file);

    virtual void processAstNode(tree ast);

    virtual void finish();

    static const int KdmTripleVersion = 1;
private:
    //typedef std::multiset<tree, DeclComparator> DeclSet;

    enum
    {
        SubjectId_Segment = 0,
        SubjectId_CodeModel,
        SubjectId_WorkbenchExtensionFamily,
        SubjectId_HiddenStereoType,
        SubjectId_CodeAssembly,
        SubjectId_PrimitiveSharedUnit,
        SubjectId_DerivedSharedUnit,
        SubjectId_ClassSharedUnit,
        SubjectId_InventoryModel,
        SubjectId_CompilationUnit,
        SubjectId_DefaultStart,
    };

    void processFunctionDeclaration(tree functionDecl);


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
    void writeSourceFile(boost::filesystem::path const & file);



    void writeCallableUnit(tree functionDecl);
    void writeParameterUnit(tree param);

    void writeTripleKdmHeader();
    void writeDefaultKdmModelElements();

    void writeKdmType(long const subject, KdmType const & object);
    void writeName(long const subject, std::string const & name);
    void writeContains(long const parent, long const child);
    void writeLinkId(long const subject, std::string const & name);




//    void writeDirectoryStructure();

   // void addPath(boost::filesystem::path const & newPath);

    KdmSinkPtr mKdmSink; /// Pointer to the kdm output stream
    long mSubjectId;     /// The current unique subject, incremented for each new subject
//    PathSet mPaths;

//    DeclSet mReferencedNodes;

};

} // namespace kdmtriplewriter

} // namespace gcckdm

#endif /* GCCKDM_KDMTRIPLEWRITER_KDMTRIPLEWRITER_HH_ */
