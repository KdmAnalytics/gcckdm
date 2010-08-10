// Copyright (c) 2010 KDM Analytics, Inc. All rights reserved.
// Date: Jul 21, 2010
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

#ifndef KDMTYPE_HH_
#define KDMTYPE_HH_

#include <string>
#include <iosfwd>


namespace gcckdm
{

class KdmType
{
private:
  enum
  {
    //Kdm Group
    KdmType_Segment,
    KdmType_ExtensionFamily,
    KdmType_StereoType,

    //Code Group
    KdmType_CodeModel,
    KdmType_CodeAssembly,
    KdmType_SharedUnit,
    KdmType_StorableUnit,
    KdmType_Value,
    KdmType_CompilationUnit,
    KdmType_CallableUnit,
    KdmType_MethodUnit,
    KdmType_ParameterUnit,
    KdmType_PointerType,
    KdmType_PrimitiveType,
    KdmType_IntegerType,
    KdmType_BooleanType,
    KdmType_DecimalType,
    KdmType_FloatType,
    KdmType_VoidType,
    KdmType_CharType,
    KdmType_Signature,
    KdmType_RecordType,
    KdmType_ClassUnit,
    KdmType_ItemUnit,
    KdmType_ArrayType,
    KdmType_LanguageUnit,
    KdmType_TypeUnit,

    //Source Group
    KdmType_InventoryModel,
    KdmType_SourceFile,
    KdmType_SourceRef,
    KdmType_SourceRegion,

    //Action Group
    KdmType_ActionElement,
    KdmType_Addresses,
    KdmType_Writes,
    KdmType_Reads,
    KdmType_BlockUnit,
    KdmType_Flow,
    KdmType_TrueFlow,
    KdmType_FalseFlow,
    KdmType_EntryFlow,
    KdmType_GuardedFlow,
    KdmType_Calls,
  };

public:
  //Kdm Group
  static const KdmType Segment()          { return KdmType(KdmType_Segment, "kdm/Segment");  }
  static const KdmType ExtensionFamily()  { return KdmType(KdmType_ExtensionFamily, "kdm/ExtensionFamily"); }
  static const KdmType StereoType()       { return KdmType(KdmType_StereoType, "kdm/StereoType"); }

  //Code Group
  static const KdmType CodeModel()        { return KdmType(KdmType_CodeModel, "code/CodeModel"); }
  static const KdmType CodeAssembly()     { return KdmType(KdmType_CodeAssembly, "code/CodeAssembly");  }
  static const KdmType SharedUnit()       { return KdmType(KdmType_SharedUnit, "code/SharedUnit");  }
  static const KdmType StorableUnit()     { return KdmType(KdmType_StorableUnit, "code/StorableUnit");  }
  static const KdmType Value()            { return KdmType(KdmType_Value, "code/Value");  }
  static const KdmType CallableUnit()     { return KdmType(KdmType_CallableUnit, "code/CallableUnit");  }
  static const KdmType MethodUnit()       { return KdmType(KdmType_MethodUnit, "code/MethodUnit");  }
  static const KdmType ParameterUnit()    { return KdmType(KdmType_ParameterUnit, "code/ParameterUnit");  }
  static const KdmType PrimitiveType()    { return KdmType(KdmType_PrimitiveType, "code/PrimitiveType");  }
  static const KdmType IntegerType()      { return KdmType(KdmType_IntegerType, "code/IntegerType");  }
  static const KdmType BooleanType()      { return KdmType(KdmType_BooleanType, "code/BooleanType");  }
  static const KdmType DecimalType()      { return KdmType(KdmType_DecimalType, "code/DecimalType");  }
  static const KdmType FloatType()        { return KdmType(KdmType_FloatType, "code/FloatType");  }
  static const KdmType VoidType()         { return KdmType(KdmType_VoidType, "code/VoidType");  }
  static const KdmType CharType()         { return KdmType(KdmType_CharType, "code/CharType");  }
  static const KdmType Signature()        { return KdmType(KdmType_Signature, "code/Signature");  }
  static const KdmType PointerType()      { return KdmType(KdmType_PointerType, "code/PointerType");  }
  static const KdmType CompilationUnit()  { return KdmType(KdmType_CompilationUnit, "code/CompilationUnit");  }
  static const KdmType RecordType()       { return KdmType(KdmType_RecordType, "code/RecordType");  }
  static const KdmType ClassUnit()        { return KdmType(KdmType_ClassUnit, "code/ClassUnit");  }
  static const KdmType ItemUnit()         { return KdmType(KdmType_ItemUnit, "code/ItemUnit");  }
  static const KdmType ArrayType()        { return KdmType(KdmType_ArrayType, "code/ArrayType");  }
  static const KdmType LanguageUnit()     { return KdmType(KdmType_LanguageUnit, "code/LanguageUnit");  }
  static const KdmType TypeUnit()         { return KdmType(KdmType_TypeUnit, "code/TypeUnit");  }

  //static const KdmType () { return KdmType(KdmType_, "code/");}
  //static const KdmType () { return KdmType(KdmType_, "code/");}

  //Source Group
  static const KdmType InventoryModel()   { return KdmType(KdmType_InventoryModel, "source/InventoryModel");  }
  static const KdmType SourceFile()       { return KdmType(KdmType_SourceFile, "source/SourceFile");  }
  static const KdmType SourceRef()        { return KdmType(KdmType_SourceRef, "source/SourceRef");  }
  static const KdmType SourceRegion()     { return KdmType(KdmType_SourceRegion, "source/SourceRegion");  }

  //Action Group
  static const KdmType ActionElement()    { return KdmType(KdmType_ActionElement, "action/ActionElement");  }
  static const KdmType Addresses()        { return KdmType(KdmType_Addresses, "action/Addresses");  }
  static const KdmType Writes()           { return KdmType(KdmType_Writes, "action/Writes");  }
  static const KdmType Reads()            { return KdmType(KdmType_Reads, "action/Reads");  }
  static const KdmType BlockUnit()        { return KdmType(KdmType_BlockUnit, "action/BlockUnit");  }
  static const KdmType Flow()             { return KdmType(KdmType_Flow, "action/Flow");  }
  static const KdmType TrueFlow()         { return KdmType(KdmType_TrueFlow, "action/TrueFlow");  }
  static const KdmType FalseFlow()        { return KdmType(KdmType_FalseFlow, "action/FalseFlow");  }
  static const KdmType EntryFlow()        { return KdmType(KdmType_EntryFlow, "action/EntryFlow");  }
  static const KdmType GuardedFlow()      { return KdmType(KdmType_GuardedFlow, "action/GuardedFlow");}
  static const KdmType Calls()            { return KdmType(KdmType_Calls, "action/Calls");  }
  //static const KdmType () { return KdmType(KdmType_, "action/");}
  //static const KdmType () { return KdmType(KdmType_, "action/");}

  int const id() const  { return mId;  }
  operator int() const  { return mId;  }
  std::string const & name() const  { return mName;  }

private:
  KdmType(int const & id, std::string name) : mId(id), mName(name)  {  }

  int mId;
  std::string mName;
};

std::ostream & operator<<(std::ostream & sink, KdmType const & pred);

} // namespace gcckdm

#endif /* KDMTYPE_HH_ */
