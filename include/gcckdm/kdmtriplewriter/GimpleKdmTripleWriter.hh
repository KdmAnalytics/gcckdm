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

/**
 * Component of the KdmTripleWriter that handles GimpleStructures
 */
class GimpleKdmTripleWriter
{
public:
    GimpleKdmTripleWriter(KdmTripleWriter & kdmTripleWriter);
    virtual ~GimpleKdmTripleWriter();

    void processGimpleSequence(tree const parent, gimple_seq const gs);
    void processGimpleStatement(tree const parent, gimple const gs);

private:
    typedef std::tr1::unordered_map<expanded_location, long, ExpanedLocationHash, ExpandedLocationEqual> LocationMap;

    long getBlockReferenceId(location_t const loc);
    void processGimpleBindStatement(tree const parent, gimple const gs);
    void processGimpleAssignStatement(tree const parent, gimple const gs);

    void processGimpleUnaryAssignStatement(long const actionId, gimple const gs);
    void processGimpleBinaryAssignStatement(long const actionId, gimple const gs);
    void processGimpleTernaryAssignStatement(long const actionId, gimple const gs);

    long writeKdmActionRelation(KdmType const & type, long const fromId, long const toId);

    /**
     * Reference to the main kdm triple writer
     */
    KdmTripleWriter & mKdmWriter;

    /**
     * Source line location to block unit id map
     */
    LocationMap mBlockUnitMap;

};

} // namespace kdmtriplewriter

} // namespace gcckdm

#endif /* GCCKDM_KDMTRIPLEWRITER_GIMPLEKDMTRIPLEWRITER_HH_ */
