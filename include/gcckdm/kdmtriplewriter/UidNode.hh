//
// Copyright (c) 2010 KDM Analytics, Inc. All rights reserved.
// Created on: Aug 12, 2010
// Author: Kyle Girard <kyle@kdmanalytics.com>
//
// This file is part of libGccKdm.
//
// Foobar is free software: you can redistribute it and/or modify
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


#ifndef GCCKDM_KDMTRIPLEWRITER_UIDNODE_HH_
#define GCCKDM_KDMTRIPLEWRITER_UIDNODE_HH_

namespace gcckdm {

namespace kdmtriplewriter {

class UidNode
{
public:
  long elementId;
  long startUid;
  long endUid;
  bool incrementFlag;
};

}  // namespace kdmtriplewriter

}  // namespace gcckdm

#endif /* GCCKDM_KDMTRIPLEWRITER_UIDNODE_HH_ */
