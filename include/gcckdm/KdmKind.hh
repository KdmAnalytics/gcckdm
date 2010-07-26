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
    KdmKind_Goto,
    KdmKind_Nop,
    KdmKind_Call,
    KdmKind_MethodCall,
    KdmKind_Equals,
    KdmKind_NotEqual,
    KdmKind_LessThanOrEqual,
    KdmKind_LessThan,
    KdmKind_GreaterThan,
    KdmKind_GreaterThanOrEqual,
    KdmKind_Not,
    KdmKind_And,
    KdmKind_Or,
    KdmKind_Xor,
    KdmKind_BitAnd,
    KdmKind_BitOr,
    KdmKind_BitNot,
    KdmKind_BitXor,
    KdmKind_LeftShift,
    KdmKind_RightShift,
    KdmKind_BitRightShift,
  };

public:

  static const KdmKind Assign()               { return KdmKind(KdmKind_Assign, "Assign");  }
  static const KdmKind Add()                  { return KdmKind(KdmKind_Add, "Add");  }
  static const KdmKind Subtract()             { return KdmKind(KdmKind_Subtract, "Subtract");  }
  static const KdmKind Multiply()             { return KdmKind(KdmKind_Multiply, "Multiply");  }
  static const KdmKind Divide()               { return KdmKind(KdmKind_Divide, "Divide");  }
  static const KdmKind Negate()               { return KdmKind(KdmKind_Negate, "Negate");  }
  static const KdmKind Return()               { return KdmKind(KdmKind_Return, "Return");  }
  static const KdmKind Register()             { return KdmKind(KdmKind_Register, "register"); }
  static const KdmKind Ptr()                  { return KdmKind(KdmKind_Ptr, "Ptr"); }
  static const KdmKind PtrReplace()           { return KdmKind(KdmKind_PtrReplace, "PtrReplace"); }
  static const KdmKind PtrSelect()            { return KdmKind(KdmKind_PtrSelect, "PtrSelect");  }
  static const KdmKind PtrCall()              { return KdmKind(KdmKind_PtrCall, "PtrCall");  }
  static const KdmKind Regular()              { return KdmKind(KdmKind_Regular, "regular");  }
  static const KdmKind Local()                { return KdmKind(KdmKind_Local, "local");  }
  static const KdmKind Condition()            { return KdmKind(KdmKind_Local, "Condition"); }
  static const KdmKind Nop()                  { return KdmKind(KdmKind_Nop, "Nop"); }
  static const KdmKind Goto()                 { return KdmKind(KdmKind_Goto, "Goto"); }
  static const KdmKind Call()                 { return KdmKind(KdmKind_Call, "Call"); }
  static const KdmKind MethodCall()           { return KdmKind(KdmKind_MethodCall, "MethodCall"); }
  static const KdmKind Equals()               { return KdmKind(KdmKind_Equals, "Equals"); }
  static const KdmKind NotEqual()             { return KdmKind(KdmKind_NotEqual, "NotEqual"); }
  static const KdmKind LessThanOrEqual()      { return KdmKind(KdmKind_LessThanOrEqual, "LessThanOrEqual"); }
  static const KdmKind LessThan()             { return KdmKind(KdmKind_LessThan, "LessThan"); }
  static const KdmKind GreaterThan()          { return KdmKind(KdmKind_GreaterThan, "GreaterThan"); }
  static const KdmKind GreaterThanOrEqual()   { return KdmKind(KdmKind_GreaterThanOrEqual, "GreaterThanOrEqual"); }
  static const KdmKind And()                  { return KdmKind(KdmKind_And, "And"); }
  static const KdmKind Or()                   { return KdmKind(KdmKind_Or, "Or"); }
  static const KdmKind Xor()                  { return KdmKind(KdmKind_Xor, "Xor"); }
  static const KdmKind BitAnd()               { return KdmKind(KdmKind_BitAnd, "BitAnd"); }
  static const KdmKind BitOr()                { return KdmKind(KdmKind_BitOr, "BitOr"); }
  static const KdmKind BitNot()               { return KdmKind(KdmKind_BitNot, "BitNot"); }
  static const KdmKind BitXor()               { return KdmKind(KdmKind_BitXor, "BitXor"); }
  static const KdmKind LeftShift()            { return KdmKind(KdmKind_LeftShift, "LeftShift"); }
  static const KdmKind RightShift()           { return KdmKind(KdmKind_RightShift, "RightShift"); }
  static const KdmKind BitRightShift()        { return KdmKind(KdmKind_BitRightShift, "BitRightShift"); }

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
