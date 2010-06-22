/*
 * KdmPredicate.cc
 *
 *  Created on: Jun 22, 2010
 *      Author: kgirard
 */

#include "gcckdm/KdmPredicate.hh"

#include <iostream>

namespace gcckdm
{

std::ostream & operator<<(std::ostream & sink, KdmPredicate const & pred)
{
    sink << pred.name();
    return sink;
}

} // namespace gcckdm
