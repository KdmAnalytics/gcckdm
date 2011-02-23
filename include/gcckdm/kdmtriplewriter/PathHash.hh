//
// Copyright (c) 2010 KDM Analytics, Inc. All rights reserved.
// Date: Jun 21, 2010
// Author: Kyle Girard <kyle@kdmanalytics.com>
//
// This file is part of libGccKdm.
//
// libGccKdm is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// libGccKdm is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with libGccKdm.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef GCCKDM_KDMTRIPLEWRITER_PATH_HASH_HH
#define GCCKDM_KDMTRIPLEWRITER_PATH_HASH_HH

#include <string>
#include <boost/filesystem/path.hpp>
#include <boost/lexical_cast.hpp>
#include "gcckdm/GccAstListener.hh"

namespace std
{

namespace tr1
{

/**
 * Template specialization of the std::tr1::hash structure
 * to allow Path variables to be hashed in std::tr1::unordered_*
 * containers.  Current defaults to the hashing on the string
 * representation of the given path
 */
template<>
struct hash<gcckdm::GccAstListener::Path>
{
  size_t operator()(gcckdm::GccAstListener::Path const & v) const
  {
    hash<gcckdm::GccAstListener::Path::string_type> h;
    return h(v.string());
  }
};

} // namespace tr1

} // namespace std


#endif
