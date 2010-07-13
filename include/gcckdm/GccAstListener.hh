/*
 * GccKdmWriter.hh
 *
 *  Created on: Jun 21, 2010
 *      Author: kgirard
 */

#ifndef GCCKDM_GCCASTLISTENER_HH_
#define GCCKDM_GCCASTLISTENER_HH_

#include "gcckdm/GccKdmConfig.hh"
#include <boost/filesystem/path.hpp>

namespace gcckdm
{

class GccAstListener
{
public:
    typedef boost::filesystem::path Path;

    virtual void startTranslationUnit(Path const & filename) = 0;
    virtual void startKdmGimplePass() = 0;
    virtual void finishKdmGimplePass() = 0;
    virtual void processAstNode(tree const ast) = 0;
    virtual void finishTranslationUnit() = 0;
};

} // namespace gcckdm

#endif /* GCCKDM_GCCASTLISTENER_HH_ */
