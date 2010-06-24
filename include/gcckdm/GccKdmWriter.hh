/*
 * GccKdmWriter.hh
 *
 *  Created on: Jun 21, 2010
 *      Author: kgirard
 */

#ifndef GCCKDM_GCCKDMWRITER_HH_
#define GCCKDM_GCCKDMWRITER_HH_

#include "gcckdm/GccKdmConfig.hh"
#include <boost/filesystem/path.hpp>

namespace gcckdm
{

class GccKdmWriter
{
public:
    virtual void startTranslationUnit(boost::filesystem::path const & filename) = 0;

    virtual void startKdmGimplePass() = 0;
    virtual void finishKdmGimplePass() = 0;

    //virtual void writeCallableUnit(tree functionDecl) = 0;

    virtual void processAstNode(tree ast) = 0;

    virtual void finishTranslationUnit() = 0;
};

} // namespace gcckdm

#endif /* GCCKDM_GCCKDMWRITER_HH_ */
