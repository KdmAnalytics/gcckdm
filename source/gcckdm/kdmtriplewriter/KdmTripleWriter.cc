// Copyright (c) 2010 KDM Analytics, Inc. All rights reserved.
// Date: Jun 21, 2010
// Author: Kyle Girard <kyle@kdmanalytics.com>, Ken Duck
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
//

#include "gcckdm/kdmtriplewriter/KdmTripleWriter.hh"

#include <algorithm>
#include <iterator>

#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/current_function.hpp>
#include <boost/format.hpp>

#include "gcckdm/GccKdmConfig.hh"
#include "gcckdm/KdmPredicate.hh"
#include "gcckdm/kdmtriplewriter/GimpleKdmTripleWriter.hh"
#include "gcckdm/kdmtriplewriter/Exception.hh"

namespace
{
std::string const commentPrefix("# ");
std::string const unsupportedPrefix("UNSUPPORTED: ");

//If a AST node doesn't have a name use this name
std::string const unamedNode("<unnamed>");

/**
 * Returns the name of the given node or the value of unnamedNode
 *
 * @param node the node to query for it's name
 *
 * @return the name of the given node or the value of unnamedNode
 */
std::string nodeName(tree const node)
{
  return gcckdm::getAstNodeName(node);
}

/**
 * True if the given AST node is an annoymous struct
 *
 * @param t the node to test
 * @return true if t is an annoymous struct
 */
bool isAnonymousStruct(tree const t)
{
  tree name = TYPE_NAME (t);
  if (name && TREE_CODE (name) == TYPE_DECL)
    name = DECL_NAME (name);
  return !name || ANON_AGGRNAME_P (name);
}

} // namespace

namespace gcckdm
{

namespace kdmtriplewriter
{

/**
 * Functor to write a comment to the kdm sink
 */
struct CommentWriter
{
  CommentWriter(KdmTripleWriter::KdmSinkPtr sink, std::string const & prefix = "") :
    mKdmSink(sink), mPrefixStr(prefix)
  {
  }

  /**
   * Writes <code>str</code> to the KdmSink
   */
  void operator()(std::string const & str)
  {
    *mKdmSink << commentPrefix << mPrefixStr << str << std::endl;
  }

  KdmTripleWriter::KdmSinkPtr mKdmSink;
  std::string mPrefixStr;
};

void writeUnsupportedComment(KdmTripleWriter::KdmSinkPtr sink, std::string const & msg)
{
  std::vector<std::string> strs;
  boost::split(strs, msg, boost::is_any_of("\n"));
  std::for_each(strs.begin(), strs.end(), CommentWriter(sink, unsupportedPrefix));
}

KdmTripleWriter::KdmTripleWriter(KdmSinkPtr const & kdmSinkPtr) : mKdmSink(kdmSinkPtr), mKdmElementId(KdmElementId_DefaultStart), mBodies(true)
{
  mGimpleWriter.reset(new GimpleKdmTripleWriter(*this));
}

KdmTripleWriter::KdmTripleWriter(Path const & filename) :
                                          mKdmSink(new boost::filesystem::ofstream(filename)), mKdmElementId(KdmElementId_DefaultStart)
{
  mGimpleWriter.reset(new GimpleKdmTripleWriter(*this));
}

KdmTripleWriter::~KdmTripleWriter()
{
  mKdmSink->flush();
}


bool KdmTripleWriter::bodies() const
{
  return mBodies;
}

void KdmTripleWriter::bodies(bool const value)
{
  mBodies = value;
}

void KdmTripleWriter::startTranslationUnit(Path const & file)
{
  try
  {
    mCompilationFile = file;
    writeVersionHeader();
    writeDefaultKdmModelElements();
    writeKdmSourceFile(mCompilationFile);
  }
  catch (KdmTripleWriterException & e)
  {
    std::cerr << boost::diagnostic_information(e);
  }
}

void KdmTripleWriter::startKdmGimplePass()
{
  try
  {
    //C Support..variables are stored in varpool... C++ we can use global_namespace
    struct varpool_node *pNode;
    for ((pNode) = varpool_nodes_queue; (pNode); (pNode) = (pNode)->next_needed)
    {
      long unitId = writeKdmStorableUnit(pNode->decl);
      writeTripleContains(getSourceFileReferenceId(pNode->decl), unitId);
    }
  }
  catch (KdmTripleWriterException & e)
  {
    std::cerr << boost::diagnostic_information(e);
  }
}

void KdmTripleWriter::finishKdmGimplePass()
{
}

void KdmTripleWriter::finishTranslationUnit()
{
  //Process any nodes that are still left on the queue
  try
  {
    for (; !mNodeQueue.empty(); mNodeQueue.pop())
    {
      processAstNode(mNodeQueue.front());
    }
  }
  catch (KdmTripleWriterException & e)
  {
    std::cerr << boost::diagnostic_information(e);
  }

  //Write any left over shared units
  try
  {
    for (TreeMap::const_iterator i = mReferencedSharedUnits.begin(), e = mReferencedSharedUnits.end(); i != e; ++i)
    {
      writeKdmSharedUnit(i->first);
    }
  }
  catch (KdmTripleWriterException & e)
  {
    std::cerr << boost::diagnostic_information(e);
  }
}

void KdmTripleWriter::processAstNode(tree const ast)
{
  try
  {
    //Ensure we haven't processed this node node before
    if (mProcessedNodes.find(ast) == mProcessedNodes.end())
    {
      int treeCode(TREE_CODE(ast));

      //All non built-in delcarations go here...
      if (DECL_P(ast) && !DECL_IS_BUILTIN(ast))
      {
        processAstDeclarationNode(ast);
      }
      else if(treeCode == NAMESPACE_DECL)
      {
        processAstNamespaceNode(ast);
      }
      else if (DECL_P(ast) && DECL_IS_BUILTIN(ast))
      {
        // SKIP all built-in declarations??
      }
      //
      else if (treeCode == LABEL_DECL)
      {
        //Not required.. . labels are handled in the gimple
      }
      else if (treeCode == TREE_LIST)
      {
        writeComment("FIXME: Skipping TREE_LIST.");
      }
      else if (treeCode == ERROR_MARK)
      {
        writeComment("FIXME: Skipping ERROR_MARK. Is this a compiler error that needs logging?");
      }
      else if (TYPE_P(ast))
      {
        processAstTypeNode(ast);
      }
      else if (treeCode == INTEGER_CST || treeCode == REAL_CST || treeCode == STRING_CST)
      {
        processAstValueNode(ast);
      }
      else
      {
        std::string msg(str(boost::format("<%3%> AST Node (%1%) in %2%") % tree_code_name[treeCode] % BOOST_CURRENT_FUNCTION % getReferenceId(ast)));
        writeUnsupportedComment(msg);
      }
      mProcessedNodes.insert(ast);
    }
  }
  catch (KdmTripleWriterException & e)
  {
    std::cerr << boost::diagnostic_information(e);
  }
}

void KdmTripleWriter::processAstDeclarationNode(tree const decl)
{
  assert(DECL_P(decl));

  int treeCode(TREE_CODE(decl));
  switch (treeCode)
  {
    case VAR_DECL:
    {
      processAstVariableDeclarationNode(decl);
      break;
    }
    case FUNCTION_DECL:
    {
      processAstFunctionDeclarationNode(decl);
      break;
    }
    case FIELD_DECL:
    {
      processAstFieldDeclarationNode(decl);
      break;
    }
    case PARM_DECL:
    {
      writeComment("FIXME: Do we need these parm_decls?");
      break;
    }
    case TYPE_DECL:
    {
      processAstTypeDecl(decl);
      break;
    }
    case LABEL_DECL:
    {
      writeComment("FIXME: We are skipping a label_decl here is it needed?");
      //      processAstLabelDeclarationNode(decl);
      break;
    }
    default:
    {
      std::string msg(str(boost::format("AST Declaration Node (%1%) in %2%") % tree_code_name[treeCode] % BOOST_CURRENT_FUNCTION));
      writeUnsupportedComment(msg);
    }
  }
}

/**
 * Type declarations may be typedefs, classes, or enumerations
 */
void KdmTripleWriter::processAstTypeDecl(tree const typeDecl)
{
  tree typeNode = TREE_TYPE (typeDecl);
  int treeCode = TREE_CODE (typeDecl);

  if(typeNode)
  {
    int typeCode = TREE_CODE(typeNode);
    if(treeCode == TYPE_DECL && typeCode == RECORD_TYPE)
    {
      // If this is artificial then it is a class declaration, otherwise a typedef.
      if(DECL_ARTIFICIAL(typeDecl))
      {
        processAstNode(typeNode);
      }
    }
  }

  // Otherwise it is a typedef
  long typedefKdmElementId = getReferenceId(typeDecl);
  writeTripleKdmType(typedefKdmElementId, KdmType::TypeUnit());

  // Get the name for the typedef, if available
  tree id (DECL_NAME (typeDecl));
  std::string name(id ? IDENTIFIER_POINTER (id) : "<unnamed>");
  writeTripleName(typedefKdmElementId, name);

  long typeKdmElementId = getReferenceId(typeNode);
  writeTriple(typedefKdmElementId, KdmPredicate::Type(), typeKdmElementId);

  writeKdmCxxContains(typeDecl);
}

void KdmTripleWriter::processAstTypeNode(tree const typeNode)
{
  assert(TYPE_P(typeNode));

  if (typeNode == TYPE_MAIN_VARIANT(typeNode))
  {
    int treeCode(TREE_CODE(typeNode));
    switch (treeCode)
    {
      case ARRAY_TYPE:
      {
        writeKdmArrayType(typeNode);
        break;
      }
      case FUNCTION_TYPE:
      {
        writeKdmSignature(typeNode);
        break;
      }
      case POINTER_TYPE:
      {
        writeKdmPointerType(typeNode);
        break;
      }
      case VOID_TYPE:
        //Fall through
      case REAL_TYPE:
        //Fall through
      case BOOLEAN_TYPE:
        //Fall through
      case INTEGER_TYPE:
      {
        writeKdmPrimitiveType(typeNode);
        break;
      }
      case UNION_TYPE:
        //Fall Through
      case RECORD_TYPE:
      {
        writeKdmRecordType(typeNode);
        break;
      }
      default:
      {
        std::string msg(str(boost::format("AST Type Node (%1%) in %2%") % tree_code_name[treeCode] % BOOST_CURRENT_FUNCTION));
        writeUnsupportedComment(msg);
        break;
      }
    }
  }
}

void KdmTripleWriter::processAstVariableDeclarationNode(tree const varDeclaration)
{
  writeKdmStorableUnit(varDeclaration);
}

void KdmTripleWriter::processAstFunctionDeclarationNode(tree const functionDecl)
{
  writeKdmCallableUnit(functionDecl);
}

void KdmTripleWriter::processAstFieldDeclarationNode(tree const fieldDecl)
{
  writeKdmItemUnit(fieldDecl);
}

//void KdmTripleWriter::processAstLabelDeclarationNode(tree const labelDecl)
//{
//  //mGimpleWriter ->//writeKdm
//}

void KdmTripleWriter::processAstValueNode(tree const val)
{
  writeKdmValue(val);
}

/**
 *
 */
void KdmTripleWriter::processAstNamespaceNode(tree const namespaceDecl)
{
  tree decl;
  cp_binding_level* level (NAMESPACE_LEVEL (namespaceDecl));

  // Traverse declarations.
  //
  for (decl = level->names;
      decl != 0;
      decl = TREE_CHAIN (decl))
  {
    if (DECL_IS_BUILTIN (decl))
      continue;

    if (!errorcount)
    {
      processAstNode(decl);
    }
  }

  // Traverse namespaces.
  //
  for(decl = level->namespaces;
      decl != 0;
      decl = TREE_CHAIN (decl))
  {
    if (DECL_IS_BUILTIN (decl))
      continue;

    if (!errorcount)
    {
      processAstNamespaceNode(decl);
    }
  }
}

void KdmTripleWriter::writeTriple(long const subject, KdmPredicate const & predicate, long const object)
{
  *mKdmSink << "<" << subject << "> <" << predicate << "> <" << object << ">.\n";
}

void KdmTripleWriter::writeTriple(long const subject, KdmPredicate const & predicate, KdmType const & object)
{
  *mKdmSink << "<" << subject << "> <" << predicate << "> \"" << object.name() << "\".\n";
}

void KdmTripleWriter::writeTriple(long const subject, KdmPredicate const & predicate, std::string const & object)
{
  *mKdmSink << "<" << subject << "> <" << predicate << "> \"" << object << "\".\n";
}

/** We do not currently use the following tests from GCCXML that provide additional information:
 *
 *   Explicit:   DECL_NONCONVERTING_P (d)
 *   Const:      DECL_CONST_MEMFUNC_P (fd)
 *   Static:     !DECL_NONSTATIC_MEMBER_FUNCTION_P (fd)
 *   Artificial: DECL_ARTIFICIAL (d)
 *   Inline:
 *   Friends:
 *   Extern:
 *
 */
void KdmTripleWriter::writeKdmCallableUnit(tree const functionDecl)
{
  std::string name(nodeName(functionDecl));
  std::string kind;

  long callableUnitId = getReferenceId(functionDecl);

  if (isFrontendCxx())
  {
    if(DECL_CONSTRUCTOR_P(functionDecl))
    {
      writeTripleKdmType(callableUnitId, KdmType::MethodUnit());
      kind.append(KdmKind::Constructor().name());
    }
    else if(DECL_DESTRUCTOR_P(functionDecl))
    {
      writeTripleKdmType(callableUnitId, KdmType::MethodUnit());
      kind.append(KdmKind::Destructor().name());
    }
    else if(DECL_OVERLOADED_OPERATOR_P(functionDecl))
    {
      writeTripleKdmType(callableUnitId, KdmType::MethodUnit());
      kind.append(KdmKind::Operator().name());
    }
    else if(DECL_FUNCTION_MEMBER_P(functionDecl))
    {
      writeTripleKdmType(callableUnitId, KdmType::MethodUnit());
      kind.append(KdmKind::Method().name());
    }
    else
    {
      writeTripleKdmType(callableUnitId, KdmType::CallableUnit());
    }
    if (DECL_VIRTUAL_P (functionDecl))
    {
      if(kind.empty()) kind.append(" ");
      kind.append(KdmKind::Virtual().name());
    }
    // C++ uses the mangled name for link:id, if possible
    if (HAS_DECL_ASSEMBLER_NAME_P(functionDecl) &&
        DECL_NAME (functionDecl) &&
        DECL_ASSEMBLER_NAME (functionDecl) &&
        DECL_ASSEMBLER_NAME (functionDecl) != DECL_NAME (functionDecl))
    {
      tree asmNode = DECL_ASSEMBLER_NAME (functionDecl);
      if(asmNode) writeTripleLinkId(callableUnitId, IDENTIFIER_POINTER (asmNode));
      else writeTripleLinkId(callableUnitId, name); // Emergency fall back
    }
    else
    {
      writeTripleLinkId(callableUnitId, name);
    }
  }
  else
  {
    writeTripleKdmType(callableUnitId, KdmType::CallableUnit());
    // Standard C does not require mangled names for link:id
    writeTripleLinkId(callableUnitId, name);
  }

  writeTripleName(callableUnitId, name);

  Path sourceFile(DECL_SOURCE_FILE(functionDecl));
  if (!sourceFile.is_complete())
  {
    sourceFile = boost::filesystem::complete(sourceFile);
  }
  long unitId(sourceFile == mCompilationFile ? KdmElementId_CompilationUnit : KdmElementId_ClassSharedUnit);

  // If this is c++, the method/function is written in the class first, otherwise in the compilation unit
  if (isFrontendCxx())
  {
    tree context = CP_DECL_CONTEXT (functionDecl);
    // If the context is a type, then get the visibility
    if(context)
    {
      if (TYPE_P(context))
      {
        if (TREE_PRIVATE (functionDecl)) writeTripleExport(callableUnitId, "private");
        else if (TREE_PROTECTED (functionDecl)) writeTripleExport(callableUnitId, "protected");
        else writeTripleExport(callableUnitId, "public");
      }
    }
    writeKdmCxxContains(functionDecl);
  }
  // In straight C, it is always written in the CompilationUnit
  else
  {
    writeTripleContains(unitId, callableUnitId);
  }
  writeKdmSourceRef(callableUnitId, functionDecl);

  long signatureId = writeKdmSignature(functionDecl);
  writeTripleContains(callableUnitId, signatureId);

  if (mBodies)
  {
    mGimpleWriter->processAstFunctionDeclarationNode(functionDecl);
  }
}

/**
 * C++ containment is more complicated then standard C containment. In KDM we follow
 * the following procedure:
 *
 *   If the context is a class, the parent is a class
 *   Otherwise the parent is the compilation/shared unit
 */
void KdmTripleWriter::writeKdmCxxContains(tree const decl)
{
  long id = getReferenceId(decl);
  if (isFrontendCxx())
  {
    tree context = CP_DECL_CONTEXT (decl);
    // If the context is a type, then get the visibility
    if(context)
    {
      // If the item belongs to a class, then the class is the parent
      int treeCode(TREE_CODE(context));
      if(treeCode == RECORD_TYPE)
      {
        long classUnitId = getReferenceId(context);
        writeTripleContains(classUnitId, id);
        return;
      }
    }
  }

  // Otherwise the file is the parent
  Path sourceFile(DECL_SOURCE_FILE(decl));
  if (!sourceFile.is_complete())
  {
    sourceFile = boost::filesystem::complete(sourceFile);
  }
  long unitId(sourceFile == mCompilationFile ? KdmElementId_CompilationUnit : KdmElementId_ClassSharedUnit);
  writeTripleContains(unitId, id);
}

long KdmTripleWriter::writeKdmSignatureDeclaration(tree const functionDecl)
{
  std::string name(nodeName(functionDecl));
  long signatureId = ++mKdmElementId;
  writeTripleKdmType(signatureId, KdmType::Signature());
  writeTripleName(signatureId, name);

  //Determine return type id
  tree t(TREE_TYPE (TREE_TYPE (functionDecl)));
  tree t2(TYPE_MAIN_VARIANT(t));
  long paramId = writeKdmReturnParameterUnit(t2);
  writeTripleContains(signatureId, paramId);

  //Iterator through argument list
  tree arg(DECL_ARGUMENTS (functionDecl));
  tree argType(TYPE_ARG_TYPES (TREE_TYPE (functionDecl)));
  while (argType && (argType != void_list_node))
  {
    if (arg)
    {
      long refId = writeKdmParameterUnit(arg);
      writeTripleContains(signatureId, refId);
      arg = TREE_CHAIN (arg);
    }
    argType = TREE_CHAIN (argType);
  }
  return signatureId;
}

long KdmTripleWriter::writeKdmSignatureType(tree const functionType)
{
  std::string name(nodeName(functionType));
  long signatureId = getReferenceId(functionType);
  writeTripleKdmType(signatureId, KdmType::Signature());
  writeTripleName(signatureId, name);

  //Determine return type
  long paramId = writeKdmReturnParameterUnit(TREE_TYPE(functionType));
  writeTripleContains(signatureId, paramId);

  //Iterator through argument list
  tree argType(TYPE_ARG_TYPES (functionType));
  while (argType && (argType != void_list_node))
  {
    long refId = writeKdmParameterUnit(argType);
    writeTripleContains(signatureId, refId);
    argType = TREE_CHAIN (argType);
  }
  writeTripleContains(KdmElementId_CompilationUnit, signatureId);

  return signatureId;

}

long KdmTripleWriter::writeKdmSignature(tree const function)
{
  long sigId;
  if (DECL_P(function))
  {
    sigId = writeKdmSignatureDeclaration(function);
  }
  else
  {
    sigId = writeKdmSignatureType(function);

  }
  return sigId;

}

void KdmTripleWriter::writeVersionHeader()
{
  *mKdmSink << "KDM_Triple:" << KdmTripleWriter::KdmTripleVersion << "\n";
}

void writeCommentWithWriter(CommentWriter const & writer, std::string const & comment)
{
  std::vector<std::string> strs;
  boost::split(strs, comment, boost::is_any_of("\n"));
  std::for_each(strs.begin(), strs.end(), writer);
}

void KdmTripleWriter::writeComment(std::string const & comment)
{
  writeCommentWithWriter(CommentWriter(mKdmSink), comment);
}

void KdmTripleWriter::writeUnsupportedComment(std::string const & comment)
{
  writeCommentWithWriter(CommentWriter(mKdmSink, unsupportedPrefix), comment);
}

void KdmTripleWriter::writeDefaultKdmModelElements()
{
  writeTriple(KdmElementId_Segment, KdmPredicate::KdmType(), KdmType::Segment());
  writeTriple(KdmElementId_Segment, KdmPredicate::Uid(), "0");
  writeTriple(KdmElementId_Segment, KdmPredicate::LinkId(), "root");
  writeTriple(KdmElementId_CodeModel, KdmPredicate::KdmType(), KdmType::CodeModel());
  writeTriple(KdmElementId_CodeModel, KdmPredicate::Name(), KdmType::CodeModel());
  writeTriple(KdmElementId_CodeModel, KdmPredicate::Uid(), "1");
  writeTriple(KdmElementId_CodeModel, KdmPredicate::LinkId(), KdmType::CodeModel());
  writeTriple(KdmElementId_Segment, KdmPredicate::Contains(), KdmElementId_CodeModel);
  writeTriple(KdmElementId_WorkbenchExtensionFamily, KdmPredicate::KdmType(), KdmType::ExtensionFamily());
  writeTriple(KdmElementId_WorkbenchExtensionFamily, KdmPredicate::Name(), "__WORKBENCH__");
  writeTriple(KdmElementId_WorkbenchExtensionFamily, KdmPredicate::LinkId(), "__WORKBENCH__");
  writeTriple(KdmElementId_Segment, KdmPredicate::Contains(), KdmElementId_WorkbenchExtensionFamily);
  writeTriple(KdmElementId_HiddenStereoType, KdmPredicate::KdmType(), KdmType::StereoType());
  writeTriple(KdmElementId_HiddenStereoType, KdmPredicate::Name(), "__HIDDEN__");
  writeTriple(KdmElementId_HiddenStereoType, KdmPredicate::LinkId(), "__HIDDEN__");
  writeTriple(KdmElementId_WorkbenchExtensionFamily, KdmPredicate::Contains(), KdmElementId_HiddenStereoType);
  writeTriple(KdmElementId_CodeAssembly, KdmPredicate::KdmType(), KdmType::CodeAssembly());
  writeTriple(KdmElementId_CodeAssembly, KdmPredicate::Name(), ":code");
  writeTriple(KdmElementId_CodeAssembly, KdmPredicate::Uid(), "2");
  writeTriple(KdmElementId_CodeAssembly, KdmPredicate::LinkId(), ":code");
  writeTriple(KdmElementId_CodeModel, KdmPredicate::Contains(), KdmElementId_CodeAssembly);
  writeTriple(KdmElementId_LanguageUnit, KdmPredicate::KdmType(), KdmType::LanguageUnit());
  writeTriple(KdmElementId_LanguageUnit, KdmPredicate::Name(), ":language");
  writeTriple(KdmElementId_LanguageUnit, KdmPredicate::Uid(), "3");
  writeTriple(KdmElementId_LanguageUnit, KdmPredicate::LinkId(), ":language");
  writeTriple(KdmElementId_CodeAssembly, KdmPredicate::Contains(), KdmElementId_LanguageUnit);
  writeTriple(KdmElementId_DerivedSharedUnit, KdmPredicate::KdmType(), KdmType::SharedUnit());
  writeTriple(KdmElementId_DerivedSharedUnit, KdmPredicate::Name(), ":derived");
  writeTriple(KdmElementId_DerivedSharedUnit, KdmPredicate::Uid(), "4");
  writeTriple(KdmElementId_DerivedSharedUnit, KdmPredicate::LinkId(), ":derived");
  writeTriple(KdmElementId_CodeAssembly, KdmPredicate::Contains(), KdmElementId_DerivedSharedUnit);
  writeTriple(KdmElementId_ClassSharedUnit, KdmPredicate::KdmType(), KdmType::SharedUnit());
  writeTriple(KdmElementId_ClassSharedUnit, KdmPredicate::Name(), ":class");
  writeTriple(KdmElementId_ClassSharedUnit, KdmPredicate::Uid(), "5");
  writeTriple(KdmElementId_ClassSharedUnit, KdmPredicate::LinkId(), ":class");
  writeTriple(KdmElementId_CodeAssembly, KdmPredicate::Contains(), KdmElementId_ClassSharedUnit);
  writeTriple(KdmElementId_InventoryModel, KdmPredicate::KdmType(), KdmType::InventoryModel());
  writeTriple(KdmElementId_InventoryModel, KdmPredicate::Name(), KdmType::InventoryModel());
  writeTriple(KdmElementId_InventoryModel, KdmPredicate::LinkId(), KdmType::InventoryModel());
  writeTriple(KdmElementId_Segment, KdmPredicate::Contains(), KdmElementId_InventoryModel);

  writeTripleKdmType(KdmElementId_CompilationUnit, KdmType::CompilationUnit());
  writeTripleName(KdmElementId_CompilationUnit, mCompilationFile.filename());
  writeTriple(KdmElementId_CompilationUnit, KdmPredicate::LinkId(), mCompilationFile.string());
  writeTriple(KdmElementId_CodeAssembly, KdmPredicate::Contains(), KdmElementId_CompilationUnit);

}

void KdmTripleWriter::writeTripleKdmType(long const subject, KdmType const & object)
{
  writeTriple(subject, KdmPredicate::KdmType(), object);
}

void KdmTripleWriter::writeTripleName(long const subject, std::string const & name)
{
  writeTriple(subject, KdmPredicate::Name(), name);
}

void KdmTripleWriter::writeTripleContains(long const parent, long const child)
{
  writeTriple(parent, KdmPredicate::Contains(), child);
}
void KdmTripleWriter::writeTripleLinkId(long const subject, std::string const & name)
{
  writeTriple(subject, KdmPredicate::LinkId(), name);
}

void KdmTripleWriter::writeTripleKind(long const subject, KdmKind const & type)
{
  writeTriple(subject, KdmPredicate::Kind(), type.name());
}

void KdmTripleWriter::writeTripleExport(long const subject, std::string const & exportName)
{
  writeTriple(subject, KdmPredicate::Export(), exportName);
}

KdmTripleWriter::FileMap::iterator KdmTripleWriter::writeKdmSourceFile(Path const & file)
{
  Path sourceFile(file);
  if (!sourceFile.is_complete())
  {
    sourceFile = boost::filesystem::complete(sourceFile);
  }

  writeTripleKdmType(++mKdmElementId, KdmType::SourceFile());
  writeTripleName(mKdmElementId, file.filename());
  writeTriple(mKdmElementId, KdmPredicate::Path(), sourceFile.string());
  writeTripleLinkId(mKdmElementId, sourceFile.string());
  writeTripleContains(KdmElementId_InventoryModel, mKdmElementId);

  //Keep track of all files inserted into the inventory model
  std::pair<FileMap::iterator, bool> result = mInventoryMap.insert(std::make_pair(sourceFile, mKdmElementId));
  return result.first;
}

void KdmTripleWriter::writeKdmCompilationUnit(Path const & file)
{
  writeTripleKdmType(++mKdmElementId, KdmType::CompilationUnit());
  writeTripleName(mKdmElementId, file.filename());
  writeTripleLinkId(mKdmElementId, file.string());
  writeTripleContains(KdmElementId_CodeAssembly, mKdmElementId);
}

long KdmTripleWriter::writeKdmReturnParameterUnit(tree const param)
{
  long ref = getReferenceId(param);
  writeTripleKdmType(++mKdmElementId, KdmType::ParameterUnit());
  writeTripleName(mKdmElementId, "__RESULT__");
  writeTriple(mKdmElementId, KdmPredicate::Type(), ref);
  return mKdmElementId;
}

long KdmTripleWriter::writeKdmParameterUnit(tree const param)
{
  if (!param)
  {
    BOOST_THROW_EXCEPTION(NullAstNodeException());
  }

  long parameterUnitId = getReferenceId(param);
  writeTripleKdmType(parameterUnitId, KdmType::ParameterUnit());

  tree type = TREE_TYPE(param) ? TYPE_MAIN_VARIANT(TREE_TYPE(param)) : TREE_VALUE (param);
  long ref = getReferenceId(type);

  std::string name(nodeName(param));
  writeTripleName(parameterUnitId, name);

  writeTriple(parameterUnitId, KdmPredicate::Type(), ref);
  return parameterUnitId;
}

long KdmTripleWriter::writeKdmItemUnit(tree const item)
{
  long itemId = getReferenceId(item);
  writeTripleKdmType(itemId, KdmType::ItemUnit());
  tree type(TYPE_MAIN_VARIANT(TREE_TYPE(item)));
  long ref = getReferenceId(type);
  std::string name(nodeName(item));

  writeTripleName(itemId, name);
  writeTriple(itemId, KdmPredicate::Type(), ref);
  writeKdmSourceRef(itemId, item);
  return itemId;
}

long KdmTripleWriter::writeKdmStorableUnit(tree const var)
{
  long unitId = getReferenceId(var);
  writeTripleKdmType(unitId, KdmType::StorableUnit());
  writeTripleName(unitId, nodeName(var));
  tree type(TYPE_MAIN_VARIANT(TREE_TYPE(var)));
  long ref = getReferenceId(type);
  writeTriple(unitId, KdmPredicate::Type(), ref);
  writeKdmSourceRef(unitId, var);

  return unitId;
}

long KdmTripleWriter::writeKdmValue(tree const val)
{
  long valueId = getReferenceId(val);
  writeTripleKdmType(valueId, KdmType::Value());
  writeTripleName(valueId, nodeName(val));
  tree type(TYPE_MAIN_VARIANT(TREE_TYPE(val)));
  long ref = getReferenceId(type);
  writeTriple(valueId, KdmPredicate::Type(), ref);
  writeTripleContains(KdmElementId_LanguageUnit, valueId);
  return valueId;
}

bool KdmTripleWriter::hasReferenceId(tree const node) const
{
  TreeMap::const_iterator i = mReferencedNodes.find(node);
  return !(i == mReferencedNodes.end());
}

long KdmTripleWriter::getReferenceId(tree const node)
{
  long retValue(-1);
  std::pair<TreeMap::iterator, bool> result = mReferencedNodes.insert(std::make_pair(node, mKdmElementId + 1));
  if (result.second)
  {
    //if we haven't processed this node before new node....queue it node for later processing
    if (mProcessedNodes.find(node) == mProcessedNodes.end())
    {
      mNodeQueue.push(node);
    }

    retValue = ++mKdmElementId;
    tree treeType(TREE_TYPE(node));
    if (treeType)
    {
      tree t2(TYPE_MAIN_VARIANT(treeType));
      //reserve the id for this type for later processing
      getReferenceId(t2);
    }
  }
  else
  {
    retValue = result.first->second;
  }
  return retValue;
}

long KdmTripleWriter::getSourceFileReferenceId(tree const t)
{
  long sourceFileId(0);
  expanded_location loc(expand_location(locationOf(t)));
  if (mCompilationFile != Path(loc.file))
  {
    tree t = get_identifier(loc.file);
    sourceFileId = getSharedUnitReferenceId(t);
  }
  else
  {
    sourceFileId = KdmElementId_CompilationUnit;
  }
  return sourceFileId;
}

long KdmTripleWriter::getSharedUnitReferenceId(tree const file)
{
  long retValue(-1);
  std::pair<TreeMap::iterator, bool> result = mReferencedSharedUnits.insert(std::make_pair(file, mKdmElementId + 1));
  if (result.second)
  {
    retValue = ++mKdmElementId;
  }
  else
  {
    retValue = result.first->second;
  }
  return retValue;
}

long KdmTripleWriter::getNextElementId()
{
  return ++mKdmElementId;
}

void KdmTripleWriter::writeKdmPrimitiveType(tree const type)
{
  long typeKdmElementId = getReferenceId(type);

  std::string name(nodeName(type));

  KdmType kdmType = KdmType::PrimitiveType();
  if (name.find("int") != std::string::npos || name.find("long") != std::string::npos)
  {
    kdmType = KdmType::IntegerType();
  }
  else if (name.find("double") != std::string::npos)
  {
    kdmType = KdmType::DecimalType();
  }
  else if (name.find("void") != std::string::npos)
  {
    kdmType = KdmType::VoidType();
  }
  else if (name.find("float") != std::string::npos)
  {
    kdmType = KdmType::FloatType();
  }
  else if (name.find("char") != std::string::npos)
  {
    kdmType = KdmType::CharType();
  }
  else if (name.find("bool") != std::string::npos)
  {
    kdmType = KdmType::BooleanType();
  }

  writeTripleKdmType(typeKdmElementId, kdmType);
  writeTripleName(typeKdmElementId, name);
  writeTripleContains(KdmElementId_LanguageUnit, typeKdmElementId);
}

void KdmTripleWriter::writeKdmPointerType(tree const pointerType)
{
  long pointerKdmElementId = getReferenceId(pointerType);
  writeTripleKdmType(pointerKdmElementId, KdmType::PointerType());
  writeTripleName(pointerKdmElementId, "PointerType");

  tree treeType(TREE_TYPE(pointerType));
  tree t2(TYPE_MAIN_VARIANT(treeType));
  long pointerTypeKdmElementId = getReferenceId(t2);
  processAstNode(t2);

  writeTriple(pointerKdmElementId, KdmPredicate::Type(), pointerTypeKdmElementId);

}

void KdmTripleWriter::writeKdmArrayType(tree const arrayType)
{
  long arrayKdmElementId = getReferenceId(arrayType);
  writeTripleKdmType(arrayKdmElementId, KdmType::ArrayType());

  tree treeType(TREE_TYPE(arrayType));
  tree t2(TYPE_MAIN_VARIANT(treeType));
  long arrayTypeKdmElementId = getReferenceId(t2);
  writeTriple(arrayKdmElementId, KdmPredicate::Type(), arrayTypeKdmElementId);

}

/** Record Types all need to output the following standard information:
 *
 *     KDM Type
 *     Name
 *     SourceRef
 *     Containment
 *     Child fields/methods/etc
 *
 */
void KdmTripleWriter::writeKdmRecordType(tree const recordType)
{
  tree mainRecordType = TYPE_MAIN_VARIANT (recordType);

  if (TREE_CODE(mainRecordType) == ENUMERAL_TYPE)
  {
    std::string msg(str(boost::format("RecordType (%1%) in %2%") % tree_code_name[TREE_CODE(mainRecordType)] % BOOST_CURRENT_FUNCTION));
    writeUnsupportedComment(msg);
    //enum
  }
  else if (global_namespace && TYPE_LANG_SPECIFIC (mainRecordType) && CLASSTYPE_DECLARED_CLASS (mainRecordType))
  {
    long compilationUnitId(getSourceFileReferenceId(mainRecordType));
    //class

    long classId = getReferenceId(mainRecordType);
    writeTripleKdmType(classId, KdmType::ClassUnit());
    std::string name;
    //check to see if we are an anonymous class
    name = (isAnonymousStruct(mainRecordType)) ? unamedNode : nodeName(mainRecordType);
    writeTripleName(classId, name);

    if (COMPLETE_TYPE_P (mainRecordType))
    {
      for (tree d(TYPE_FIELDS(mainRecordType)); d; d = TREE_CHAIN(d))
      {
        switch (TREE_CODE(d))
        {
          case TYPE_DECL:
          {
            if (!DECL_SELF_REFERENCE_P(d))
            {
              std::string msg(str(boost::format("RecordType (%1%) in %2%") % tree_code_name[TREE_CODE(d)] % BOOST_CURRENT_FUNCTION));
              writeUnsupportedComment(msg);
            }
            break;
          }
          case FIELD_DECL:
          {
            if (!DECL_ARTIFICIAL(d))
            {
              long itemId = getReferenceId(d);
              processAstNode(d);
              writeTripleContains(classId, itemId);
            }
            break;
          }
          default:
          {
            std::string msg(str(boost::format("RecordType (%1%) in %2%") % tree_code_name[TREE_CODE(d)] % BOOST_CURRENT_FUNCTION));
            writeUnsupportedComment(msg);
            break;
          }
        }
      }
    }

    writeKdmSourceRef(classId, mainRecordType);

    writeTripleContains(compilationUnitId, classId);
  }
  else //Record or Union

  {
    long compilationUnitId(getSourceFileReferenceId(mainRecordType));

    //struct
    long structId = getReferenceId(mainRecordType);
    writeTripleKdmType(structId, KdmType::RecordType());
    std::string name;
    //check to see if we are an anonymous struct
    name = (isAnonymousStruct(mainRecordType)) ? unamedNode : nodeName(mainRecordType);
    writeTripleName(structId, name);

    if (COMPLETE_TYPE_P (mainRecordType))
    {
      for (tree d(TYPE_FIELDS(mainRecordType)); d; d = TREE_CHAIN(d))
      {
        switch (TREE_CODE(d))
        {
          case TYPE_DECL:
          {
            if (!DECL_SELF_REFERENCE_P(d))
            {
              std::string msg(str(boost::format("RecordType (%1%) in %2%") % tree_code_name[TREE_CODE(d)] % BOOST_CURRENT_FUNCTION));
              writeUnsupportedComment(msg);
            }
            break;
          }
          case FIELD_DECL:
          {
            if (!DECL_ARTIFICIAL(d))
            {
              long itemId = getReferenceId(d);
              processAstNode(d);
              writeTripleContains(structId, itemId);
            }
            break;
          }
          default:
          {
            std::string msg(str(boost::format("RecordType (%1%) in %2%") % tree_code_name[TREE_CODE(d)] % BOOST_CURRENT_FUNCTION));
            writeUnsupportedComment(msg);
            break;
          }
        }
      }

    }

    writeKdmSourceRef(structId, mainRecordType);

    writeTripleContains(compilationUnitId, structId);
  }
  //    std::cerr << "======================= End of Record Type\n";

}

void KdmTripleWriter::writeKdmSharedUnit(tree const file)
{
  long id = getSharedUnitReferenceId(file);
  writeTripleKdmType(id, KdmType::SharedUnit());

  Path filename(IDENTIFIER_POINTER(file));
  writeTripleName(id, filename.filename());
  writeTripleLinkId(id, filename.string());
  writeTripleContains(KdmElementId_CodeAssembly, id);
}

long KdmTripleWriter::writeKdmSourceRef(long id, const tree node)
{
  return writeKdmSourceRef(id, expand_location(locationOf(node)));
}

long KdmTripleWriter::writeKdmSourceRef(long id, const expanded_location & eloc)
{
  Path sourceFile(eloc.file);
  if (!sourceFile.is_complete())
  {
    sourceFile = boost::filesystem::complete(sourceFile);
  }

  FileMap::iterator i = mInventoryMap.find(sourceFile);
  if (i == mInventoryMap.end())
  {
    i = writeKdmSourceFile(sourceFile);
  }
  std::string srcRef = boost::lexical_cast<std::string>(i->second) + ";" + boost::lexical_cast<std::string>(eloc.line);
  writeTriple(id, KdmPredicate::SourceRef(), srcRef);

  return id;
}

} // namespace kdmtriplewriter

}
// namespace gcckdm
