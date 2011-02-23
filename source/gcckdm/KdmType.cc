//
// Copyright (c) 2010 KDM Analytics, Inc. All rights reserved.
// Date: Jun 22, 2010
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

#include "gcckdm/KdmType.hh"

#include <iostream>

namespace gcckdm
{

std::ostream & operator<<(std::ostream & sink, KdmType const & pred)
{
  sink << pred.name();
  return sink;
}

} // namespace gcckdm
