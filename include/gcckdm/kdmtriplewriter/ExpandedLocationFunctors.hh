//
// Copyright (c) 2010 KDM Analytics, Inc. All rights reserved.
// Date: Jul 13, 2010
// Author: Kyle Girard <kyle@kdmanalytics.com>
//
// This file is part of libGccKdm.
//
// libGccKdm is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// libGccKdm is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with libGccKdm.  If not, see <http://www.gnu.org/licenses/>.
//

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
    std::tr1::hash<std::string> h;
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
