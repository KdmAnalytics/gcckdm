/*
 * GccKdmWriter.hh
 *
 *  Created on: Jul 13, 2010
 *      Author: kgirard
 */

#ifndef GCCKDM_KDMTRIPLEWRITER_TRIPLEWRITER_HH_
#define GCCKDM_KDMTRIPLEWRITER_TRIPLEWRITER_HH_

#include "gcckdm/kdmtriplewriter/KdmTripleWriterFwd.hh"


namespace gcckdm
{

namespace kdmtriplewriter
{

/**
 * Interface for all triple writers
 */
class TripleWriter
{
public:

    /**
     *
     */
    virtual ~TripleWriter(){};

    /**
     *
     */
    virtual void writeTriple(long const subject, KdmPredicate const & predicate, long const object) = 0;

    /**
     *
     */
    virtual void writeTriple(long const subject, KdmPredicate const & predicate, KdmType const & object) = 0;

    /**
     *
     */
    virtual void writeTriple(long const subject, KdmPredicate const & predicate, std::string const & object) = 0;

private:
};

} // namespace kdmtriplewriter

} // namespace gcckdm
#endif /* GCCKDM_KDMTRIPLEWRITER_TRIPLEWRITER_HH_ */
