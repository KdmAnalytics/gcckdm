/*
 * KdmTripleWriter.hh
 *
 *  Created on: Jun 21, 2010
 *      Author: kgirard
 */

#ifndef GCCKDM_KDMTRIPLEWRITER_KDMTRIPLEWRITER_HH_
#define GCCKDM_KDMTRIPLEWRITER_KDMTRIPLEWRITER_HH_

#include <iostream>

#include "boost/shared_ptr.hpp"

#include "gcckdm/GccKdmWriter.hh"
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
    virtual void finish();

    static const int KdmTripleVersion = 1;
private:

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
        SubjectId_DefaultStart
    };

    void writeTripleKdmHeader();
    void writeDefaultKdmModelElements();

    void writeTriple(long const & subject, KdmPredicate const & predicate, long const & object);
    void writeTriple(long const & subject, KdmPredicate const & predicate, KdmType const & object);
    void writeTriple(long const & subject, KdmPredicate const & predicate, std::string const & object);

    void writeSourceFile(boost::filesystem::path const & file);

    KdmSinkPtr mKdmSink;
    long mSubjectId;

};

} // namespace kdmtriplewriter

} // namespace gcckdm

#endif /* GCCKDM_KDMTRIPLEWRITER_KDMTRIPLEWRITER_HH_ */
