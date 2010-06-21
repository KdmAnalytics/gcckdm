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
#include "boost/filesystem/path.hpp"

#include "gcckdm/GccKdmWriter.hh"

namespace gcckdm
{

namespace kdmtriplewriter
{

class KdmTripleWriter: public GccKdmWriter
{
public:
    typedef boost::shared_ptr<std::ostream> KdmSinkPtr;

    KdmTripleWriter(KdmSinkPtr const & kdmSink);
    KdmTripleWriter(boost::filesystem::path const & filename);

    virtual void start();
    virtual void finish();

private:
    KdmSinkPtr mKdmSink;
};

} // namespace kdmtriplewriter

} // namespace gcckdm

#endif /* GCCKDM_KDMTRIPLEWRITER_KDMTRIPLEWRITER_HH_ */
