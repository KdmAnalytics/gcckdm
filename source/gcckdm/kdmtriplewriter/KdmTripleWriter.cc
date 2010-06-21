/*
 * KdmTripleWriter.cc
 *
 *  Created on: Jun 21, 2010
 *      Author: kgirard
 */

#include "gcckdm/kdmtriplewriter/KdmTripleWriter.hh"

#include <fstream>
#include "boost/filesystem/fstream.hpp"


namespace gcckdm
{

namespace kdmtriplewriter
{
KdmTripleWriter::KdmTripleWriter(KdmSinkPtr const & kdmSinkPtr) :
    mKdmSink(kdmSinkPtr)
{

}

KdmTripleWriter::KdmTripleWriter(boost::filesystem::path const & filename)
 : mKdmSink(new boost::filesystem::ofstream(filename))
{
}

void KdmTripleWriter::start()
{
    *mKdmSink << "UM output the Segment\n";
}

void KdmTripleWriter::finish()
{
    *mKdmSink << "We are all done here" << std::endl; ;
}

} // namespace kdmtriplewriter

} // namespace gcckdm
