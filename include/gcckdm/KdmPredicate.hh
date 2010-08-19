//
// Copyright (c) 2010 KDM Analytics, Inc. All rights reserved.
// Date: Jul 21, 2010
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

#ifndef KDMPREDICATE_HH_
#define KDMPREDICATE_HH_

#include <string>
#include <iosfwd>

namespace gcckdm
{

class KdmPredicate
{
private:
  enum
  {
    KdmPredicate_KdmType,
    KdmPredicate_Contains,
    KdmPredicate_Type,
    KdmPredicate_Name,
    KdmPredicate_Kind,
    KdmPredicate_From,
    KdmPredicate_To,
    KdmPredicate_Path,
    KdmPredicate_LinkId,
    KdmPredicate_SourceRef,
    KdmPredicate_Uid,
    KdmPredicate_LastUid,
    KdmPredicate_Export,
    KdmPredicate_Stereotype,
  };

public:
  static const KdmPredicate KdmType()
  {
    return KdmPredicate(KdmPredicate_KdmType, "kdmType");
  }
  static const KdmPredicate Contains()
  {
    return KdmPredicate(KdmPredicate_Contains, "contains");
  }
  static const KdmPredicate Type()
  {
    return KdmPredicate(KdmPredicate_Type, "type");
  }
  static const KdmPredicate Name()
  {
    return KdmPredicate(KdmPredicate_Name, "name");
  }
  static const KdmPredicate Kind()
  {
    return KdmPredicate(KdmPredicate_Kind, "kind");
  }
  static const KdmPredicate From()
  {
    return KdmPredicate(KdmPredicate_From, "from");
  }
  static const KdmPredicate To()
  {
    return KdmPredicate(KdmPredicate_To, "to");
  }
  static const KdmPredicate Path()
  {
    return KdmPredicate(KdmPredicate_Path, "path");
  }
  static const KdmPredicate SourceRef()
  {
    return KdmPredicate(KdmPredicate_SourceRef, "SourceRef");
  }

  static const KdmPredicate LinkId()
  {
    return KdmPredicate(KdmPredicate_LinkId, "link::id");
  }
  static const KdmPredicate Uid()
  {
    return KdmPredicate(KdmPredicate_Uid, "UID");
  }
  static const KdmPredicate LastUid()
  {
    return KdmPredicate(KdmPredicate_LastUid, "lastUID");
  }
  static const KdmPredicate Export()
  {
    return KdmPredicate(KdmPredicate_Export, "export");
  }
  static const KdmPredicate Stereotype()
  {
    return KdmPredicate(KdmPredicate_Stereotype, "stereotype");
  }

  int const id() const
  {
    return mId;
  }
  operator int() const
  {
    return mId;
  }
  std::string const & name() const
  {
    return mName;
  }

private:
  KdmPredicate(int const & id, std::string name) :
    mId(id), mName(name)
  {
  }

  int mId;
  std::string mName;
};

std::ostream & operator<<(std::ostream & sink, KdmPredicate const & pred);

} // namespace gcckdm

#endif /* KDMPREDICATE_HH_ */
