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
//
// libGccKdm is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with libGccKdm.  If not, see <http://www.gnu.org/licenses/>.

#ifndef KDMTYPE_HH_
#define KDMTYPE_HH_

#include <string>
#include <iosfwd>


namespace gcckdm
{

namespace kdm
{

class Type
{
private:
  enum
  {
    //Kdm Group
    Type_Segment,
#if 1 //BBBB
    Type_Audit,
#endif
    Type_ExtensionFamily,
    Type_StereoType,

    //Code Group
    Type_CodeModel,
    Type_CodeAssembly,
    Type_SharedUnit,
    Type_StorableUnit,
    Type_Value,
    Type_CompilationUnit,
    Type_CallableUnit,
    Type_MethodUnit,
    Type_MemberUnit,
    Type_ParameterUnit,
    Type_PointerType,
    Type_PrimitiveType,
    Type_IntegerType,
    Type_BooleanType,
    Type_DecimalType,
    Type_FloatType,
    Type_VoidType,
    Type_CharType,
    Type_Signature,
    Type_RecordType,
    Type_ClassUnit,
    Type_ItemUnit,
    Type_ArrayType,
    Type_LanguageUnit,
    Type_TypeUnit,
    Type_Extends,
    Type_CodeRelationship,
    Type_EnumeratedType,
    Type_Package,
    Type_HasType,
    Type_HasValue,
    Type_TemplateUnit,
    Type_TemplateParameter,
    Type_TemplateType,
    Type_InstanceOf,

    //Source Group
    Type_InventoryModel,
    Type_SourceFile,
    Type_SourceRef,
    Type_SourceRegion,
    Type_Directory,

    //Action Group
    Type_ActionElement,
    Type_Addresses,
    Type_Invokes,
    Type_Writes,
    Type_Reads,
    Type_UsesType,
    Type_BlockUnit,
    Type_Flow,
    Type_TrueFlow,
    Type_FalseFlow,
    Type_EntryFlow,
    Type_GuardedFlow,
    Type_ExceptionFlow,
    Type_ExitFlow,
    Type_Calls,
    Type_CompliesTo,
    Type_TryUnit,
    Type_CatchUnit,
    Type_FinallyUnit
  };

public:
  //Kdm Group
  static const Type Segment()          { return Type(Type_Segment, "kdm/Segment");  }
#if 1 //BBBB
  static const Type Audit()            { return Type(Type_Audit, "kdm/Audit");  }
#endif
  static const Type ExtensionFamily()  { return Type(Type_ExtensionFamily, "kdm/ExtensionFamily"); }
  static const Type Stereotype()       { return Type(Type_StereoType, "kdm/Stereotype"); }

  //Code Group
  static const Type CodeModel()        { return Type(Type_CodeModel, "code/CodeModel"); }
  static const Type CodeAssembly()     { return Type(Type_CodeAssembly, "code/CodeAssembly");  }
  static const Type SharedUnit()       { return Type(Type_SharedUnit, "code/SharedUnit");  }
  static const Type StorableUnit()     { return Type(Type_StorableUnit, "code/StorableUnit");  }
  static const Type Value()            { return Type(Type_Value, "code/Value");  }
  static const Type CallableUnit()     { return Type(Type_CallableUnit, "code/CallableUnit");  }
  static const Type MethodUnit()       { return Type(Type_MethodUnit, "code/MethodUnit");  }
  static const Type MemberUnit()       { return Type(Type_MemberUnit, "code/MemberUnit");  }
  static const Type ParameterUnit()    { return Type(Type_ParameterUnit, "code/ParameterUnit");  }
  static const Type PrimitiveType()    { return Type(Type_PrimitiveType, "code/PrimitiveType");  }
  static const Type IntegerType()      { return Type(Type_IntegerType, "code/IntegerType");  }
  static const Type BooleanType()      { return Type(Type_BooleanType, "code/BooleanType");  }
  static const Type DecimalType()      { return Type(Type_DecimalType, "code/DecimalType");  }
  static const Type FloatType()        { return Type(Type_FloatType, "code/FloatType");  }
  static const Type VoidType()         { return Type(Type_VoidType, "code/VoidType");  }
  static const Type CharType()         { return Type(Type_CharType, "code/CharType");  }
  static const Type Signature()        { return Type(Type_Signature, "code/Signature");  }
  static const Type PointerType()      { return Type(Type_PointerType, "code/PointerType");  }
  static const Type CompilationUnit()  { return Type(Type_CompilationUnit, "code/CompilationUnit");  }
  static const Type RecordType()       { return Type(Type_RecordType, "code/RecordType");  }
  static const Type ClassUnit()        { return Type(Type_ClassUnit, "code/ClassUnit");  }
  static const Type ItemUnit()         { return Type(Type_ItemUnit, "code/ItemUnit");  }
  static const Type ArrayType()        { return Type(Type_ArrayType, "code/ArrayType");  }
  static const Type LanguageUnit()     { return Type(Type_LanguageUnit, "code/LanguageUnit");  }
  static const Type TypeUnit()         { return Type(Type_TypeUnit, "code/TypeUnit");  }
  static const Type Extends()          { return Type(Type_Extends, "code/Extends");  }
  static const Type CodeRelationship() { return Type(Type_CodeRelationship, "code/CodeRelationship");  }
  static const Type EnumeratedType()   { return Type(Type_EnumeratedType, "code/EnumeratedType"); }
  static const Type Package()          { return Type(Type_Package, "code/Package"); }
  static const Type HasType()          { return Type(Type_HasType, "code/HasType"); }
  static const Type HasValue()         { return Type(Type_HasValue, "code/HasValue"); }
  static const Type TemplateUnit()     { return Type(Type_TemplateUnit, "code/TemplateUnit");}
  static const Type TemplateParameter(){ return Type(Type_TemplateParameter, "code/TemplateParameter");}
  static const Type TemplateType()     { return Type(Type_TemplateType, "code/TemplateType");}
  static const Type InstanceOf()       { return Type(Type_InstanceOf, "code/InstanceOf");}

  //static const KdmType () { return KdmType(Type_, "code/");}
  //static const KdmType () { return KdmType(Type_, "code/");}

  //Source Group
  static const Type InventoryModel()   { return Type(Type_InventoryModel, "source/InventoryModel");  }
  static const Type SourceFile()       { return Type(Type_SourceFile, "source/SourceFile");  }
  static const Type SourceRef()        { return Type(Type_SourceRef, "source/SourceRef");  }
  static const Type SourceRegion()     { return Type(Type_SourceRegion, "source/SourceRegion");  }
  static const Type Directory()        { return Type(Type_SourceRegion, "source/Directory");  }

  //Action Group
  static const Type ActionElement()    { return Type(Type_ActionElement, "action/ActionElement");  }
  static const Type Addresses()        { return Type(Type_Addresses, "action/Addresses");  }
  static const Type Invokes()          { return Type(Type_Invokes, "action/Invokes");  }
  static const Type Writes()           { return Type(Type_Writes, "action/Writes");  }
  static const Type Reads()            { return Type(Type_Reads, "action/Reads");  }
  static const Type UsesType()         { return Type(Type_UsesType, "action/UsesType");  }
  static const Type BlockUnit()        { return Type(Type_BlockUnit, "action/BlockUnit");  }
  static const Type Flow()             { return Type(Type_Flow, "action/Flow");  }
  static const Type TrueFlow()         { return Type(Type_TrueFlow, "action/TrueFlow");  }
  static const Type FalseFlow()        { return Type(Type_FalseFlow, "action/FalseFlow");  }
  static const Type EntryFlow()        { return Type(Type_EntryFlow, "action/EntryFlow");  }
  static const Type GuardedFlow()      { return Type(Type_GuardedFlow, "action/GuardedFlow"); }
  static const Type ExceptionFlow()    { return Type(Type_ExceptionFlow, "action/ExceptionFlow"); }
  static const Type ExitFlow()         { return Type(Type_ExitFlow, "action/ExitFlow"); }
  static const Type Calls()            { return Type(Type_Calls, "action/Calls");  }
  static const Type CompliesTo()       { return Type(Type_CompliesTo, "action/CompliesTo");  }
  static const Type TryUnit()          { return Type(Type_TryUnit, "action/TryUnit"); }
  static const Type CatchUnit()        { return Type(Type_CatchUnit, "action/CatchUnit"); }
  static const Type FinallyUnit()      { return Type(Type_FinallyUnit, "action/FinallyUnit"); }
  static const Type Dispatches()       { return Type(Type_FinallyUnit, "action/Dispatches"); }
  //static const KdmType () { return KdmType(Type_, "action/");}
  //static const KdmType () { return KdmType(Type_, "action/");}

  int const id() const  { return mId;  }
  operator int() const  { return mId;  }
  std::string const & name() const  { return mName;  }

private:
  Type(int const & id, std::string name) : mId(id), mName(name)  {  }

  int mId;
  std::string mName;
};

std::ostream & operator<<(std::ostream & sink, Type const & pred);

} // namespace kdm

} // namespace gcckdm

#endif /* KDMTYPE_HH_ */
