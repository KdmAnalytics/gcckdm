/*
 * GccKdmWriter.hh
 *
 *  Created on: Jul 13, 2010
 *      Author: kgirard
 */

#ifndef GCCKDM_KDMTRIPLEWRITER_TRIPLEWRITER_HH_
#define GCCKDM_KDMTRIPLEWRITER_TRIPLEWRITER_HH_

#include <string>
#include <gcckdm/KdmPredicate.hh>
#include <gcckdm/KdmType.hh>


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
     * Empty Virtual Destructor to allow proper destruction
     */
    virtual ~TripleWriter(){};

    /**
     * Writes the given three values as a triple
     *
     * @param subject - the subject id
     * @param predicate - the KDM predicate
     * @param object - the object id
     */
    virtual void writeTriple(long const subject, KdmPredicate const & predicate, long const object) = 0;

    /**
     * Writes the given three values as a triple
     *
     * @param subject - the subject id
     * @param predicate - the KDM predicate
     * @param object - a KdmType as defined by the KDM specification ie action/ActionElement
     */
    virtual void writeTriple(long const subject, KdmPredicate const & predicate, KdmType const & object) = 0;

    /**
     * Writes the given three values as a triple
     *
     * @param subject - the subject id
     * @param predicate - the KDM predicate
     * @param object - a free form string (literal)
     */
    virtual void writeTriple(long const subject, KdmPredicate const & predicate, std::string const & object) = 0;

private:
};

} // namespace kdmtriplewriter

} // namespace gcckdm
#endif /* GCCKDM_KDMTRIPLEWRITER_TRIPLEWRITER_HH_ */
