//
// Copyright (c) 2010 KDM Analytics, Inc. All rights reserved.
// Date: Jul 16, 2010
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

#ifndef GCCKDM_KDM_KIND_HH_
#define GCCKDM_KDM_KIND_HH_

#include <gcckdm/kdm/IKind.hh>
#include <string>

namespace gcckdm
{

namespace kdm
{

class Kind : public IKind
{
private:
  enum
  {
    //Micro Kdm Group
    Kind_Assign,
    Kind_Add,
    Kind_Subtract,
    Kind_Multiply,
    Kind_Divide,
    Kind_Remainder,
    Kind_Negate,
    Kind_Return,
    Kind_Register,
    Kind_Ptr,
    Kind_PtrReplace,
    Kind_PtrSelect,
    Kind_PtrCall,
    Kind_Local,
    Kind_Condition,
    Kind_Goto,
    Kind_Nop,
    Kind_Call,
    Kind_MethodCall,
    Kind_Equals,
    Kind_NotEqual,
    Kind_LessThanOrEqual,
    Kind_LessThan,
    Kind_GreaterThan,
    Kind_GreaterThanOrEqual,
    Kind_Not,
    Kind_And,
    Kind_Or,
    Kind_Xor,
    Kind_BitAnd,
    Kind_BitOr,
    Kind_BitNot,
    Kind_BitXor,
    Kind_LeftShift,
    Kind_RightShift,
    Kind_BitRightShift,
    Kind_FieldSelect,
    Kind_FieldReplace,
    Kind_ChoiceSelect,
    Kind_ChoiceReplace,
    Kind_ArraySelect,
    Kind_ArrayReplace,
    Kind_MemberSelect,
    Kind_MemberReplace,
    Kind_New,
    Kind_NewArray,
    Kind_VirtualCall,
    Kind_Throw,
    Kind_Incr,
    Kind_Decr,
    Kind_Switch,
    Kind_Compound,
    Kind_Sizeof,
    Kind_InstanceOf,
    Kind_DynCast,
    Kind_TypeCast,
    Kind_Exception,
    Kind_CatchAll,
    Kind_Init,

    // Method kinds
    Kind_Method,
    Kind_Constructor,
    Kind_Destructor,
    Kind_Virtual,
    Kind_Abstract,

    // Library Kinds
    Kind_Asm, //Not in KdmSpec
    Kind_BitAssign, //Not in KdmSpec
    Kind_RightRotate, // Not in KdmSpec
    Kind_LeftRotate, // Not in KdmSpec
    Kind_Case // Not in KdmSpec
  };

public:

  static const Kind Assign()               { return Kind(Kind_Assign, "Assign");  }
  static const Kind Add()                  { return Kind(Kind_Add, "Add");  }
  static const Kind Subtract()             { return Kind(Kind_Subtract, "Subtract");  }
  static const Kind Multiply()             { return Kind(Kind_Multiply, "Multiply");  }
  static const Kind Divide()               { return Kind(Kind_Divide, "Divide");  }
  static const Kind Remainder()            { return Kind(Kind_Remainder, "Remainder");  }
  static const Kind Negate()               { return Kind(Kind_Negate, "Negate");  }
  static const Kind Return()               { return Kind(Kind_Return, "Return");  }
  static const Kind Register()             { return Kind(Kind_Register, "register"); }
  static const Kind Ptr()                  { return Kind(Kind_Ptr, "Ptr"); }
  static const Kind PtrReplace()           { return Kind(Kind_PtrReplace, "PtrReplace"); }
  static const Kind PtrSelect()            { return Kind(Kind_PtrSelect, "PtrSelect");  }
  static const Kind PtrCall()              { return Kind(Kind_PtrCall, "PtrCall");  }
  static const Kind Local()                { return Kind(Kind_Local, "local");  }
  static const Kind Static()               { return Kind(Kind_Local, "static");  }
  static const Kind Global()               { return Kind(Kind_Local, "global");  }
  static const Kind Condition()            { return Kind(Kind_Local, "Condition"); }
  static const Kind Nop()                  { return Kind(Kind_Nop, "Nop"); }
  static const Kind Goto()                 { return Kind(Kind_Goto, "Goto"); }
  static const Kind Call()                 { return Kind(Kind_Call, "Call"); }
  static const Kind MethodCall()           { return Kind(Kind_MethodCall, "MethodCall"); }
  static const Kind Equals()               { return Kind(Kind_Equals, "Equals"); }
  static const Kind NotEqual()             { return Kind(Kind_NotEqual, "NotEqual"); }
  static const Kind LessThanOrEqual()      { return Kind(Kind_LessThanOrEqual, "LessThanOrEqual"); }
  static const Kind LessThan()             { return Kind(Kind_LessThan, "LessThan"); }
  static const Kind GreaterThan()          { return Kind(Kind_GreaterThan, "GreaterThan"); }
  static const Kind GreaterThanOrEqual()   { return Kind(Kind_GreaterThanOrEqual, "GreaterThanOrEqual"); }
  static const Kind Not()                  { return Kind(Kind_Not, "Not"); }
  static const Kind And()                  { return Kind(Kind_And, "And"); }
  static const Kind Or()                   { return Kind(Kind_Or, "Or"); }
  static const Kind Xor()                  { return Kind(Kind_Xor, "Xor"); }
  static const Kind BitAnd()               { return Kind(Kind_BitAnd, "BitAnd"); }
  static const Kind BitOr()                { return Kind(Kind_BitOr, "BitOr"); }
  static const Kind BitNot()               { return Kind(Kind_BitNot, "BitNot"); }
  static const Kind BitXor()               { return Kind(Kind_BitXor, "BitXor"); }
  static const Kind LeftShift()            { return Kind(Kind_LeftShift, "LeftShift"); }
  static const Kind RightShift()           { return Kind(Kind_RightShift, "RightShift"); }
  static const Kind BitRightShift()        { return Kind(Kind_BitRightShift, "BitRightShift"); }
  static const Kind FieldSelect()          { return Kind(Kind_FieldSelect, "FieldSelect"); }
  static const Kind FieldReplace()         { return Kind(Kind_FieldReplace, "FieldReplace"); }
  static const Kind ChoiceSelect()         { return Kind(Kind_ChoiceSelect, "ChoiceSelect"); }
  static const Kind ChoiceReplace()        { return Kind(Kind_ChoiceReplace, "ChoiceReplace"); }
  static const Kind ArraySelect()          { return Kind(Kind_ArraySelect, "ArraySelect"); }
  static const Kind ArrayReplace()         { return Kind(Kind_ArrayReplace, "ArrayReplace"); }
  static const Kind MemberSelect()         { return Kind(Kind_MemberSelect, "MemberSelect"); }
  static const Kind MemberReplace()        { return Kind(Kind_MemberReplace, "MemberReplace"); }
  static const Kind New()                  { return Kind(Kind_New, "New"); }
  static const Kind NewArray()             { return Kind(Kind_NewArray, "NewArray"); }
  static const Kind VirtualCall()          { return Kind(Kind_VirtualCall, "VirtualCall"); }
  static const Kind Throw()                { return Kind(Kind_Throw, "Throw"); }
  static const Kind Incr()                 { return Kind(Kind_Incr, "Incr"); }
  static const Kind Decr()                 { return Kind(Kind_Decr, "Decr"); }
  static const Kind Switch()               { return Kind(Kind_Switch, "Switch"); }
  static const Kind Compound()             { return Kind(Kind_Compound, "Compound"); }
  static const Kind Sizeof()               { return Kind(Kind_Sizeof, "Sizeof"); }
  static const Kind InstanceOf()           { return Kind(Kind_InstanceOf, "InstanceOf"); }
  static const Kind DynCast()              { return Kind(Kind_DynCast, "DynCast"); }
  static const Kind TypeCast()             { return Kind(Kind_TypeCast, "TypeCast"); }
  static const Kind Exception()            { return Kind(Kind_Exception, "Exception"); }
  static const Kind CatchAll()             { return Kind(Kind_CatchAll, "CatchAll"); }
  static const Kind Init()                 { return Kind(Kind_Init, "Init");  }

  // Method Kind
  static const Kind Method()               { return Kind(Kind_Method, "method"); }
  static const Kind Constructor()          { return Kind(Kind_Constructor, "constructor"); }
  static const Kind Destructor()           { return Kind(Kind_Destructor, "destructor"); }
  static const Kind Virtual()              { return Kind(Kind_Virtual, "virtual"); }
  static const Kind Abstract()             { return Kind(Kind_Abstract, "abstract"); }

  // Library Kinds
  static const Kind Asm()                  { return Kind(Kind_Asm, "Asm");  } //Not in KdmSpec
  static const Kind BitAssign()            { return Kind(Kind_BitAssign, "BitAssign");  } //Not in KdmSpec
  static const Kind LeftRotate()           { return Kind(Kind_LeftRotate, "LeftRotate"); } //Not in KdmSpec
  static const Kind RightRotate()          { return Kind(Kind_RightRotate, "RightRotate"); } //Not in KdmSpec
  static const Kind Case()                 { return Kind(Kind_Case, "Case"); }

  int const id() const
  {
    return mId;
  }
  operator int() const
  {
    return mId;
  }


  virtual std::string const & name() const
  {
    return mName;
  }

private:
  Kind(int const & id, std::string name) :
    mId(id), mName(name)
  {
  }

  int mId;
  std::string mName;
};

} //namespace kdm

} // namespace gcckdm

#endif /* GCCKDM_KDM_KIND_HH_ */
