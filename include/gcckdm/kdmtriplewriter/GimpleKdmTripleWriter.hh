/*
 * KdmTripleGimpleWriter.hh
 *
 *  Created on: Jul 13, 2010
 *      Author: kgirard
 */

#ifndef GCCKDM_KDMTRIPLEWRITER_GIMPLEKDMTRIPLEWRITER_HH_
#define GCCKDM_KDMTRIPLEWRITER_GIMPLEKDMTRIPLEWRITER_HH_

#include <tr1/unordered_map>
#include "gcckdm/GccKdmConfig.hh"
#include "gcckdm/GccKdmUtilities.hh"

#include "gcckdm/kdmtriplewriter/KdmTripleWriterFwd.hh"
#include "gcckdm/kdmtriplewriter/ExpandedLocationFunctors.hh"



namespace gcckdm
{

namespace kdmtriplewriter
{

class GimpleKdmTripleWriter
{
public:
    GimpleKdmTripleWriter(KdmTripleWriter & kdmTripleWriter);
    ~GimpleKdmTripleWriter();

    void processGimpleSequence(tree const parent, gimple_seq const gs);
    void processGimpleStatement(tree const parent, gimple const gs);

private:
    typedef std::tr1::unordered_map<expanded_location, long, ExpanedLocationHash, ExpandedLocationEqual> LocationMap;

    long getBlockReferenceId(location_t const loc);


    void processGimpleBindStatement(tree const parent, gimple const gs);
    void processGimpleAssignStatement(tree const parent, gimple const gs);

    KdmTripleWriter & mKdmWriter;
    LocationMap mBlockUnitMap;

};
}
} // namespace gcckdm

#endif /* GCCKDM_KDMTRIPLEWRITER_GIMPLEKDMTRIPLEWRITER_HH_ */
