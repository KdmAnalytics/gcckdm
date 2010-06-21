/*
 * GccKdmWriter.hh
 *
 *  Created on: Jun 21, 2010
 *      Author: kgirard
 */

#ifndef GCCKDM_GCCKDMWRITER_HH_
#define GCCKDM_GCCKDMWRITER_HH_

namespace gcckdm {

class GccKdmWriter
{
public:
    virtual void start() = 0;
    virtual void finish() = 0;
};

}  // namespace gcckdm

#endif /* GCCKDM_GCCKDMWRITER_HH_ */
