/*
 * KdmType.cc
 *
 *  Created on: Jun 22, 2010
 *      Author: kgirard
 */

#include "gcckdm/KdmType.hh"

#include <iostream>

namespace gcckdm {

std::ostream & operator<<(std::ostream & sink, KdmType const & pred)
{
    sink << pred.name();
    return sink;
}


}  // namespace gcckdm
