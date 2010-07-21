//
// Copyright (c) 2010 KDM Analytics, Inc. All rights reserved.
// Date: Jul 16, 2010
// Author: Kyle Girard <kyle@kdmanalytics.com>
//
// This file is part of libGccKdm.
//
// Foobar is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// libGccKdm is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with libGccKdm.  If not, see <http://www.gnu.org/licenses/>.

#ifndef GCCKDM_KDMKIND_HH_
#define GCCKDM_KDMKIND_HH_

namespace gcckdm
{

class KdmKind
{
private:
private:
  enum
  {
    //Kdm Group
    KdmKind_Assign,
    KdmKind_Add,
    KdmKind_Subtract,
    KdmKind_Multiply,
    KdmKind_Divide,
    KdmKind_Negate,
    KdmKind_Return,
    KdmKind_Register,
    KdmKind_Ptr,
    KdmKind_PtrReplace,
    KdmKind_PtrSelect,
    KdmKind_PtrCall,
    KdmKind_Regular,
    KdmKind_Local,
    KdmKind_Condition,
  };

public:
  static const KdmKind Assign()
  {
    return KdmKind(KdmKind_Assign, "Assign");
  }
  static const KdmKind Add()
  {
    return KdmKind(KdmKind_Add, "Add");
  }
  static const KdmKind Subtract()
  {
    return KdmKind(KdmKind_Subtract, "Subtract");
  }
  static const KdmKind Multiply()
  {
    return KdmKind(KdmKind_Multiply, "Multiply");
  }
  static const KdmKind Divide()
  {
    return KdmKind(KdmKind_Divide, "Divide");
  }
  static const KdmKind Negate()
  {
    return KdmKind(KdmKind_Negate, "Negate");
  }
  static const KdmKind Return()
  {
    return KdmKind(KdmKind_Return, "Return");
  }
  static const KdmKind Register()
  {
    return KdmKind(KdmKind_Register, "register");
  }
  static const KdmKind Ptr()
  {
    return KdmKind(KdmKind_Ptr, "Ptr");
  }
  static const KdmKind PtrReplace()
  {
    return KdmKind(KdmKind_PtrReplace, "PtrReplace");
  }
  static const KdmKind PtrSelect()
  {
    return KdmKind(KdmKind_PtrSelect, "PtrSelect");
  }
  static const KdmKind PtrCall()
  {
    return KdmKind(KdmKind_PtrCall, "PtrCall");
  }
  static const KdmKind Regular()
  {
    return KdmKind(KdmKind_Regular, "regular");
  }
  static const KdmKind Local()
  {
    return KdmKind(KdmKind_Local, "local");
  }
  static const KdmKind Condition()
  {
    return KdmKind(KdmKind_Local, "Condition");
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
  KdmKind(int const & id, std::string name) :
    mId(id), mName(name)
  {
  }

  int mId;
  std::string mName;
};

} // namespace gcckdm

#endif /* GCCKDM_KDMKIND_HH_ */
