/*
 * ExpandedLocationEqual.hh
 *
 *  Created on: Jul 13, 2010
 *      Author: kgirard
 */

#ifndef GCCKDM_KDMTRIPLEWRITER_EXPANDEDLOCATIONFUNCTORS_HH_
#define GCCKDM_KDMTRIPLEWRITER_EXPANDEDLOCATIONFUNCTORS_HH_

#include <string>
#include <boost/lexical_cast.hpp>

namespace gcckdm
{

namespace kdmtriplewriter
{

struct ExpanedLocationHash
{
    size_t operator()(expanded_location const & v) const
    {
        std::tr1::hash < std::string > h;
        return h(std::string(v.file) + boost::lexical_cast<std::string>(v.line));
    }
};

struct ExpandedLocationEqual
{
    bool operator()(expanded_location const & rhs, expanded_location const & lhs) const
    {
        return (std::string(rhs.file) == std::string(lhs.file)) && (rhs.line == lhs.line);
    }
};

} // namespace kdmtriplewriter

} // namespace gcckdm

#endif /* GCCKDM_KDMTRIPLEWRITER_EXPANDEDLOCATIONFUNCTORS_HH_ */
