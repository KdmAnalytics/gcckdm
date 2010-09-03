// Copyright (c) 2010 KDM Analytics, Inc. All rights reserved.
// Date: Jun 21, 2010
// Author: Kyle Girard <kyle@kdmanalytics.com>, Ken Duck
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
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/graphviz.hpp>
#include <fstream>
namespace
{
/// Prefix for all KDM triple format comments
std::string const commentPrefix("# ");

/// Prefix for all KDM triple unsupported node statements, used in conjunction with commentPrefix
std::string const unsupportedPrefix("UNSUPPORTED: ");

//If a AST node doesn't have a name use this name
std::string const unnamedNode("<unnamed>");

/**
 * Returns the name of the given node or the value of unnamedNode
 *
 * @param node the node to query for it's name
 *
 * @return the name of the given node or the value of unnamedNode
 */
std::string nodeName(tree const node)
{
  std::string name;
  if (node == NULL_TREE)
  {
    name = unnamedNode;
  }
  else
  {
    name = gcckdm::getAstNodeName(node);
    if (name.empty())
    {
      name = unnamedNode;
    }
  }
  return name;
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
  /**
   * Constructs a CommentWriter which writes to the given sink and prefixes
   * every comment line written with the given prefix
   */
  CommentWriter(KdmTripleWriter::KdmSinkPtr sink, std::string const & prefix = "") :
    mKdmSink(sink), mPrefixStr(prefix)
  {
  }

  /**
   * Writes <code>str</code> to the KdmSink
   *
   * @param the str a string to write to the kdmsink
   */
  void operator()(std::string const & str)
  {
    *mKdmSink << commentPrefix << mPrefixStr << str << std::endl;
  }

  KdmTripleWriter::KdmSinkPtr mKdmSink;
  std::string mPrefixStr;
};

/**
 * For every node visited write it's UID and lastUID
 *
 * For use with boost::depth_first_search
 */
class KdmTripleWriter::UidVisitor : public boost::default_dfs_visitor
{
public:
  /**
   * Constructs a visitor that can write the required triples using the
   * given writer and modify the graph to include new/updated values
   *
   * Have to include the graph as a non-const reference here to
   * allow updating of vertex properties.  The graph that is passed
   * to the functions while visiting is a constant graph.  While
   * const_cast could be used because we know that we aren't modifying
   * the graph in this visitor I prefer to avoid casting when possible
   *
   * @param the writer to use to write triples
   * @param graph the non-const instance of the graph to modify while visiting
   */
  UidVisitor(KdmTripleWriter & writer, UidGraph & graph)
  : mWriter(writer),
    mGraph(graph)
  {
  }

  void discover_vertex(KdmTripleWriter::Vertex v, KdmTripleWriter::UidGraph const & g)
  {
    //KdmTripleWriter::UidGraph & gg= const_cast<KdmTripleWriter::UidGraph&>(g);
    //std::cerr << "Start:" << g[v].elementId << std::endl;
    mGraph[v].startUid = mWriter.mUid++;
    mLastVertex = v;
  }

  void finish_vertex(KdmTripleWriter::Vertex v, KdmTripleWriter::UidGraph const & g)
  {
    //KdmTripleWriter::UidGraph & gg= const_cast<KdmTripleWriter::UidGraph&>(g);
    //    std::cerr << "Finish:" << g[v].elementId << std::endl;
    //A leaf mark uid's identical
    if (mLastVertex == v)
    {
      mGraph[v].endUid = mGraph[v].startUid;
    }
    else
    {
      mGraph[v].endUid = mGraph[mLastVertex].endUid;
    }

    mWriter.writeTriple(mGraph[v].elementId, KdmPredicate::Uid(), boost::lexical_cast<std::string>(mGraph[v].startUid));

    //Don't write lastUid if the uid's are the same
    if (mGraph[v].elementId != mGraph[v].endUid)
    {
      mWriter.writeTriple(mGraph[v].elementId, KdmPredicate::LastUid(), boost::lexical_cast<std::string>(mGraph[v].endUid));
    }
  }
private:
  KdmTripleWriter & mWriter;
  KdmTripleWriter::UidGraph & mGraph;
  KdmTripleWriter::Vertex mLastVertex;
};


/**
 * Tokenizes the given string by looking for newline characters writes each token
 * as a seperate unsupported comment using the CommentWriter function object
 *
 * @param sink the destination of the comment
 * @param msg the string to output with the unsupported prefix
 */
void writeUnsupportedComment(KdmTripleWriter::KdmSinkPtr sink, std::string const & msg)
{
  std::vector<std::string> strs;
  boost::split(strs, msg, boost::is_any_of("\n"));
  std::for_each(strs.begin(), strs.end(), CommentWriter(sink, unsupportedPrefix));
}



KdmTripleWriter::KdmTripleWriter(KdmSinkPtr const & kdmSinkPtr, KdmTripleWriter::Settings const & settings)
: mKdmSink(kdmSinkPtr),
  mKdmElementId(KdmElementId_DefaultStart),
  mUidGraph(),
  mUid(0),
  mSettings(settings)
{
  mGimpleWriter.reset(new GimpleKdmTripleWriter(*this));
}

KdmTripleWriter::KdmTripleWriter(Path const & filename, KdmTripleWriter::Settings const & settings)
: mKdmElementId(KdmElementId_DefaultStart),
  mUidGraph(),
  mUid(0),
  mSettings(settings)
{
  if (settings.outputDir.filename().empty())
  {
    mKdmSink.reset(new boost::filesystem::ofstream(filename));
  }
  else
  {
    mKdmSink.reset(new boost::filesystem::ofstream(mSettings.outputDir / filename.filename()));
  }

  mGimpleWriter.reset(new GimpleKdmTripleWriter(*this));
}

KdmTripleWriter::~KdmTripleWriter()
{
  mKdmSink->flush();
}


void KdmTripleWriter::startTranslationUnit(Path const & file)
{
  try
  {
    //Ensure we hav the complete path
    mCompilationFile = (!file.is_complete()) ? boost::filesystem::complete(file) : file;
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
      if (!hasReferenceId(pNode->decl))
      {
        long unitId = writeKdmStorableUnit(pNode->decl);
        writeTripleContains(getSourceFileReferenceId(pNode->decl), unitId);
      }
    }
  }
  catch (KdmTripleWriterException & e)
  {
    std::cerr << boost::diagnostic_information(e);
  }
}

void KdmTripleWriter::finishKdmGimplePass()
{
  processNodeQueue();
}


void KdmTripleWriter::processNodeQueue()
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
}

void KdmTripleWriter::writeReferencedSharedUnits()
{
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

class VertexPropertyWriter
{
public:
  typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, UidNode> Graph;
  VertexPropertyWriter(Graph& graph) : mGraph(graph)
  {

  }

  template <class Vertex>
  void operator()(std::ostream& out, const Vertex v) const
  {
    out << "[label=\"" << mGraph[v].elementId << "\"]";
  }

private:
  Graph & mGraph;
};

void KdmTripleWriter::writeUids()
{
  //Calculate and Write UIDs

  //First we try to locate our root node
  //it should be the segment but we do this just in case.....
  boost::graph_traits<UidGraph>::vertex_iterator i, end;
  bool foundFlag = false;
  for (boost::tie(i, end) = boost::vertices(mUidGraph); i != end; ++i)
  {
    if (0 == mUidGraph[*i].elementId)
    {
      foundFlag = true;
      break;
    }
  }


  if (foundFlag)
  {
    //Perform a depth_first_search using the segment as our starting point
    KdmTripleWriter::UidVisitor vis(*this, mUidGraph);
    boost::depth_first_search(mUidGraph, boost::visitor(vis).root_vertex(*i));
  }
  else
  {
    writeComment("Unable to locate root of UID tree, no UIDs will be written");
  }

  if (mSettings.generateUidGraph)
  {
    //  Use this to view the graph in dot
    std::ofstream out("graph.viz", std::ios::out);
    boost::write_graphviz(out, mUidGraph, VertexPropertyWriter(mUidGraph));
  }
}

void KdmTripleWriter::finishTranslationUnit()
{
  processNodeQueue();
  //writeReferencedSharedUnits();
  if (mSettings.generateUids)
  {
    writeUids();
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
        // FIXME: Is this the correct handling for the tree_list?
        tree l = ast;
        while(l)
        {
          //          tree treePurpose = TREE_PURPOSE(l);
          tree treeValue = TREE_VALUE(l);
          if(treeValue)
          {
            processAstNode(treeValue);
          }
          l = TREE_CHAIN (l);
        }
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
      else if (treeCode == COMPONENT_REF)
      {
        std::string msg(str(boost::format("<%3%> AST Node (%1%) in %2%:%4%") % tree_code_name[treeCode] % BOOST_CURRENT_FUNCTION % getReferenceId(ast) % __LINE__));
        writeUnsupportedComment(msg);
      }
      else
      {
        std::string msg(str(boost::format("<%3%> AST Node. Name=(%1%) code=(%5%) in %2%:%4%") % tree_code_name[treeCode] % BOOST_CURRENT_FUNCTION % getReferenceId(ast) % __LINE__ % treeCode));
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
      return;
    }
    case TYPE_DECL:
    {
      processAstTypeDecl(decl);
      break;
    }
    case CONST_DECL:
    {
      //For the moment we ignore these.  Gimple seems to sub in the
      //actual value anyways
      break;
    }
    case TEMPLATE_DECL:
    {
      processAstTemplateDecl(decl);
      break;
    }
    case RESULT_DECL:
    {
      //Ignore for the moment.  In tree-pretty-print.c they simply print "<retval>"
      //It's possible that we should do something with this but for now.....
      break;
    }
    case LABEL_DECL:
    {
      writeComment("FIXME: We are skipping a label_decl here is it needed?");
      //      processAstLabelDeclarationNode(decl);
      return;
    }
    default:
    {
      std::string msg(str(boost::format("AST Declaration Node (%1%) in %2%") % tree_code_name[treeCode] % BOOST_CURRENT_FUNCTION));
      writeUnsupportedComment(msg);
      return;
    }
  }

  writeKdmTypeQualifiers(decl);
}

/**
 *
 */
void KdmTripleWriter::writeKdmTypeQualifiers(tree const decl)
{
  long id = getReferenceId(decl);
  // FIXME: This code was intended to detect constant functions. It does not currently work
  if(DECL_NONSTATIC_MEMBER_FUNCTION_P (decl))
  {
    tree treeType = TREE_TYPE (decl);
    tree typeArgsType = TYPE_ARG_TYPES (treeType);
    tree treeValue = TREE_VALUE (typeArgsType);
    tree treeValueType = TREE_TYPE (treeValue);

    int qualifiers = gcckdm::getTypeQualifiers(treeValueType);
    if(qualifiers & TYPE_QUAL_CONST)
    {
      writeTriple(id, KdmPredicate::Stereotype(), KdmElementId_ConstStereotype);
    }
  }

  // Type qualifiers: Assemble any applicable Stereotypes that supply additional information
  int qualifiers = gcckdm::getTypeQualifiers(decl);
  if(qualifiers & TYPE_QUAL_CONST)
  {
    writeTriple(id, KdmPredicate::Stereotype(), KdmElementId_ConstStereotype);
  }
  if(qualifiers & TYPE_QUAL_VOLATILE)
  {
    writeTriple(id, KdmPredicate::Stereotype(), KdmElementId_VolatileStereotype);
  }
  if(qualifiers & TYPE_QUAL_RESTRICT)
  {
    writeTriple(id, KdmPredicate::Stereotype(), KdmElementId_RestrictStereotype);
  }
  if (DECL_MUTABLE_P (decl))
  {
    writeTriple(id, KdmPredicate::Stereotype(), KdmElementId_MutableStereotype);
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

  // Otherwise it is a typedef...
  long typedefKdmElementId = getReferenceId(typeDecl);
  writeTripleKdmType(typedefKdmElementId, KdmType::TypeUnit());

  // Get the name for the typedef, if available
  tree id (DECL_NAME (typeDecl));
  writeTripleName(typedefKdmElementId, nodeName(id));

  long typeKdmElementId = getReferenceId(typeNode);
  writeTriple(typedefKdmElementId, KdmPredicate::Type(), typeKdmElementId);

  // Write the containment information
  writeKdmCxxContains(typeDecl);
}

/**
 *
 */
void KdmTripleWriter::processAstTemplateDecl(tree const templateDecl)
{
  // Dump the template specializations.
  for (tree tl = DECL_TEMPLATE_SPECIALIZATIONS (templateDecl); tl ; tl = TREE_CHAIN (tl))
  {
    tree ts = TREE_VALUE (tl);
    int treeCode = TREE_CODE (ts);
    switch (treeCode)
    {
      case FUNCTION_DECL:
      {
        writeComment("--- Start Specialization");
        processAstNode(ts);
        writeComment("--- End Specialization");
        break;
      }
      //      case TEMPLATE_DECL:
      //        break;
      default:
      {
        std::string msg(str(boost::format("AST Template Specialization Node (%1%) in %2%") % tree_code_name[treeCode] % BOOST_CURRENT_FUNCTION));
        writeUnsupportedComment(msg);
        break;
      }
    }
  }

  /* Dump the template instantiations.  */
  for (tree tl = DECL_TEMPLATE_INSTANTIATIONS (templateDecl); tl ; tl = TREE_CHAIN (tl))
  {
    tree ts = TYPE_NAME (TREE_VALUE (tl));
    int treeCode = TREE_CODE (ts);
    switch (treeCode)
    {
      case TYPE_DECL:
      {
        // GCCXML only processed the node in some circumstances. Do we need to do the same?
        //        /* Add the instantiation only if it is real.  */
        //        if (!xml_find_template_parm (TYPE_TI_ARGS(TREE_TYPE(ts))))
        //        {
        //          xml_add_node (xdi, ts, complete);
        //        }
        processAstNode(ts);
        break;
      }
      default:
      {
        std::string msg(str(boost::format("AST Template Instantiation Node (%1%) in %2%") % tree_code_name[treeCode] % BOOST_CURRENT_FUNCTION));
        writeUnsupportedComment(msg);
        break;
      }
    }
  }

  /* Dump any member template instantiations.  */
  if(TREE_CODE (TREE_TYPE (templateDecl)) == RECORD_TYPE ||
      TREE_CODE (TREE_TYPE (templateDecl)) == UNION_TYPE ||
      TREE_CODE (TREE_TYPE (templateDecl)) == QUAL_UNION_TYPE)
  {
    for (tree tl = TYPE_FIELDS (TREE_TYPE (templateDecl)); tl; tl = TREE_CHAIN (tl))
    {
      int treeCode = TREE_CODE (tl);
      if (treeCode == TEMPLATE_DECL)
      {
        std::string msg(str(boost::format("AST Template Instantiation Node (%1%) in %2%") % tree_code_name[treeCode] % BOOST_CURRENT_FUNCTION));
        writeUnsupportedComment(msg);
        //        processAstNode(tl);
      }
    }
  }
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
        //writeKdmSignature(typeNode);
        break;
      }
      case METHOD_TYPE:
      {
        //writeKdmSignature(typeNode);
        break;
      }
      case REFERENCE_TYPE:
        writeComment("NOTE: This is actually a reference type, not a pointer type does that matter for KDM?");
        //Fall Through
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
      case ENUMERAL_TYPE:
      {
        writeEnumType(typeNode);
        break;
      }
      case UNION_TYPE:
        //Fall Through
      case RECORD_TYPE:
      {
        processAstRecordTypeNode(typeNode);
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

void KdmTripleWriter::processAstRecordTypeNode(tree const typeNode)
{
  int treeCode(TREE_CODE(typeNode));
  if (isFrontendCxx())
  {
    // The contained code taken from GCCXML
    if (TYPE_PTRMEMFUNC_P (typeNode))
    {
      // Pointer-to-member-functions are stored in a RECORD_TYPE.
      std::string msg(str(boost::format("AST Type Node pointer to member-function (%1%) in %2%") % tree_code_name[treeCode] % BOOST_CURRENT_FUNCTION));
      writeUnsupportedComment(msg);
    }
    else if (!CLASSTYPE_IS_TEMPLATE (typeNode))
    {
      // This is a struct or class type.
      writeKdmRecordType(typeNode);
    }
    else
    {
      // This is a class template.  We don't want to dump it.
      std::string msg(str(boost::format("AST Type Node Class Template (%1%) in %2%") % tree_code_name[treeCode] % BOOST_CURRENT_FUNCTION));
      writeUnsupportedComment(msg);
    }
  }
  else
  {
    // This is a struct or class type.
    writeKdmRecordType(typeNode);
  }
}

void KdmTripleWriter::writeEnumType(tree const enumType)
{
  long enumId = getReferenceId(enumType);
  writeTripleKdmType(enumId, KdmType::EnumeratedType());
  writeTripleName(enumId, nodeName(enumType));

  for (tree tv = TYPE_VALUES (enumType); tv ; tv = TREE_CHAIN (tv))
  {
    if(TREE_CODE (TREE_VALUE (tv)) == INTEGER_CST)
    {
      long valueId = getReferenceId(tv);
      int value = TREE_INT_CST_LOW (TREE_VALUE (tv));
      writeTripleKdmType(valueId, KdmType::Value());
      writeTripleName(valueId, boost::lexical_cast<std::string>(value));
      writeTriple(valueId, KdmPredicate::Type(), KdmType::IntegerType());
      writeTriple(valueId, KdmPredicate::EnumName(), nodeName(TREE_PURPOSE(tv)));
      writeTripleContains(enumId, valueId);
    }
    else
    {
        int treeCode(TREE_CODE(TREE_VALUE (tv)));
        std::string msg(str(boost::format("AST Type Node Enum (%1%) in %2%") % tree_code_name[treeCode] % BOOST_CURRENT_FUNCTION));
        writeUnsupportedComment(msg);
    }
  }
  long compilationUnitId = getSourceFileReferenceId(enumType);
  writeTripleContains(compilationUnitId, enumId);
}


void KdmTripleWriter::processAstVariableDeclarationNode(tree const varDeclaration)
{
  writeKdmStorableUnit(varDeclaration);
}

void KdmTripleWriter::processAstFunctionDeclarationNode(tree const functionDecl)
{
  writeKdmCallableUnit(functionDecl);
}

/**
 * In C++ a field may be a MemberUnit or an ItemUnit. We determine this by looking at
 * the context.
 */
void KdmTripleWriter::processAstFieldDeclarationNode(tree const fieldDecl)
{
  if (isFrontendCxx())
  {
    tree context = CP_DECL_CONTEXT (fieldDecl);
    // If the context is a type, then get the visibility
    if(context)
    {
      // If the item belongs to a class, then the class is the parent
      int treeCode(TREE_CODE(context));
      if(treeCode == RECORD_TYPE)
      {
        writeKdmMemberUnit(fieldDecl);
        return;
      }
    }
  }
  writeKdmItemUnit(fieldDecl);
}

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

/**
 *
 */
void KdmTripleWriter::writeKdmCallableUnit(tree const functionDecl)
{
  std::string name(nodeName(functionDecl));

  long callableUnitId = getReferenceId(functionDecl);

  if (isFrontendCxx())
  {
    if(DECL_CONSTRUCTOR_P(functionDecl))
    {
      writeTripleKdmType(callableUnitId, KdmType::MethodUnit());
      writeTripleKind(callableUnitId, KdmKind::Constructor());
    }
    else if(DECL_DESTRUCTOR_P(functionDecl))
    {
      writeTripleKdmType(callableUnitId, KdmType::MethodUnit());
      writeTripleKind(callableUnitId, KdmKind::Destructor());
    }
    else if(DECL_OVERLOADED_OPERATOR_P(functionDecl))
    {
      writeTripleKdmType(callableUnitId, KdmType::MethodUnit());
      writeTripleKind(callableUnitId, KdmKind::Operator());
    }
    else if(DECL_FUNCTION_MEMBER_P(functionDecl))
    {
      writeTripleKdmType(callableUnitId, KdmType::MethodUnit());
      writeTripleKind(callableUnitId, KdmKind::Method());
    }
    else
    {
      writeTripleKdmType(callableUnitId, KdmType::CallableUnit());
      if(DECL_REALLY_EXTERN(functionDecl)) writeTripleKind(callableUnitId, KdmKind::External());
      else writeTripleKind(callableUnitId, KdmKind::Regular());
    }
    // First check for pure virtual, then virtual. No need to mark pure virtual functions as both
    if (DECL_PURE_VIRTUAL_P (functionDecl))
    {
      // As described in KDM-1, using a Stereotype to mark virtual functions instead of the "kind"
      writeTriple(callableUnitId, KdmPredicate::Stereotype(), KdmElementId_PureVirtualStereotype);
    }
    else if (DECL_VIRTUAL_P (functionDecl))
    {
      // As described in KDM-1, using a Stereotype to mark virtual functions instead of the "kind"
      writeTriple(callableUnitId, KdmPredicate::Stereotype(), KdmElementId_VirtualStereotype);
    }
    // C++ uses the mangled name for link:id, if possible
    writeTripleLinkId(callableUnitId, gcckdm::getLinkId(functionDecl, name));
  }
  else
  {
    writeTripleKdmType(callableUnitId, KdmType::CallableUnit());
    //    if(DECL_REALLY_EXTERN(functionDecl))
    if (DECL_EXTERNAL(functionDecl))
      writeTripleKind(callableUnitId, KdmKind::External());
    else
      writeTripleKind(callableUnitId, KdmKind::Regular());
    // Standard C does not require mangled names for link:id
    writeTripleLinkId(callableUnitId, name);
  }

  writeTripleName(callableUnitId, name);

  // Inline keyword
  if (DECL_DECLARED_INLINE_P (functionDecl))
    writeTriple(callableUnitId, KdmPredicate::Stereotype(), KdmElementId_InlineStereotype);

  // Const keyword
  writeKdmTypeQualifiers(functionDecl);

  // Static keyword
  if (!DECL_NONSTATIC_MEMBER_FUNCTION_P (functionDecl))
    writeTriple(callableUnitId, KdmPredicate::Stereotype(), KdmElementId_StaticStereotype);

  // Source reference
  writeKdmSourceRef(callableUnitId, functionDecl);

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

    // Explicit keyword
    if (DECL_NONCONVERTING_P (functionDecl))
      writeTriple(callableUnitId, KdmPredicate::Stereotype(), KdmElementId_ExplicitStereotype);

    // Explicit keyword
    if (DECL_ARTIFICIAL (functionDecl))
      writeTriple(callableUnitId, KdmPredicate::Stereotype(), KdmElementId_HiddenStereotype);

    // Friendship
    writeKdmTripleFriends(callableUnitId, DECL_BEFRIENDING_CLASSES(functionDecl));

    // Containment
    writeKdmCxxContains(functionDecl);
  }
  // In straight C, it is always contained in the source file
  else
  {
    //    expanded_location loc(expand_location(locationOf(functionDecl)));
    //    Path file(loc.file);
    //    if (!file.is_complete())
    //    {
    //      file = boost::filesystem::complete(file);
    //    }
    //    std::cerr << "HERE: " << " " << DECL_EXTERNAL(functionDecl) << " " << TREE_PUBLIC(functionDecl)<< " " << gcckdm::getAstNodeName(functionDecl) << ": " << file.string() << std::endl;

    long unitId = getSourceFileReferenceId(functionDecl);
    writeTripleContains(unitId, callableUnitId);
  }

  long signatureId = writeKdmSignature(functionDecl);
  writeTripleContains(callableUnitId, signatureId);

  if (mSettings.functionBodies)
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
      writeKdmTypeQualifiers(arg);
      writeTripleContains(signatureId, refId);
      arg = TREE_CHAIN (arg);
    }
    argType = TREE_CHAIN (argType);
  }

  // Throws
  if(gcckdm::isFrontendCxx())
  {
    tree ft = TREE_TYPE (functionDecl);
    tree raises = TYPE_RAISES_EXCEPTIONS (ft);
    if(raises)
    {
      if(TREE_VALUE (raises))
      {
        for (;raises != NULL_TREE; raises = TREE_CHAIN (raises))
        {
          long raisesId = getReferenceId(TREE_VALUE (raises));
          long throwId = writeKdmThrows(raisesId);
          writeTripleContains(signatureId, throwId);
        }
      }
    }
  }
  return signatureId;
}

/**
 *
 */
long KdmTripleWriter::writeKdmThrows(long const id)
{
  writeTripleKdmType(++mKdmElementId, KdmType::ParameterUnit());
//  writeTripleName(mKdmElementId, "__RESULT__");
  writeTriple(mKdmElementId, KdmPredicate::Type(), id);
  writeTripleKind(mKdmElementId, KdmKind::Throw());
  return mKdmElementId;
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
  tree argType = TYPE_ARG_TYPES (functionType);
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
  // Segment and CodeModel definitions
  writeTriple(KdmElementId_Segment, KdmPredicate::KdmType(), KdmType::Segment());
  writeTriple(KdmElementId_Segment, KdmPredicate::LinkId(), "root");
  writeTriple(KdmElementId_CodeModel, KdmPredicate::KdmType(), KdmType::CodeModel());
  writeTriple(KdmElementId_CodeModel, KdmPredicate::Name(), KdmType::CodeModel());
  writeTriple(KdmElementId_CodeModel, KdmPredicate::LinkId(), KdmType::CodeModel());
  writeTripleContains(KdmElementId_Segment, KdmElementId_CodeModel);

  // Workbench Stereotypes
  writeTriple(KdmElementId_WorkbenchExtensionFamily, KdmPredicate::KdmType(), KdmType::ExtensionFamily());
  writeTriple(KdmElementId_WorkbenchExtensionFamily, KdmPredicate::Name(), "__WORKBENCH__");
  writeTriple(KdmElementId_WorkbenchExtensionFamily, KdmPredicate::LinkId(), "__WORKBENCH__");
  writeTripleContains(KdmElementId_Segment, KdmElementId_WorkbenchExtensionFamily);
  writeTriple(KdmElementId_HiddenStereotype, KdmPredicate::KdmType(), KdmType::Stereotype());
  writeTriple(KdmElementId_HiddenStereotype, KdmPredicate::Name(), "__HIDDEN__");
  writeTriple(KdmElementId_HiddenStereotype, KdmPredicate::LinkId(), "__HIDDEN__");
  writeTripleContains(KdmElementId_WorkbenchExtensionFamily, KdmElementId_HiddenStereotype);

  // C++ Stereotypes
  writeTriple(KdmElementId_WorkbenchExtensionFamily, KdmPredicate::KdmType(), KdmType::ExtensionFamily());
  writeTriple(KdmElementId_WorkbenchExtensionFamily, KdmPredicate::Name(), "C/C++");
  writeTriple(KdmElementId_WorkbenchExtensionFamily, KdmPredicate::LinkId(), "C/C++");
  writeTripleContains(KdmElementId_Segment, KdmElementId_CxxExtensionFamily);

  writeTriple(KdmElementId_MutableStereotype, KdmPredicate::KdmType(), KdmType::Stereotype());
  writeTriple(KdmElementId_MutableStereotype, KdmPredicate::Name(), "mutable");
  writeTriple(KdmElementId_MutableStereotype, KdmPredicate::LinkId(), "mutable");
  writeTripleContains(KdmElementId_CxxExtensionFamily, KdmElementId_MutableStereotype);

  writeTriple(KdmElementId_VolatileStereotype, KdmPredicate::KdmType(), KdmType::Stereotype());
  writeTriple(KdmElementId_VolatileStereotype, KdmPredicate::Name(), "volatile");
  writeTriple(KdmElementId_VolatileStereotype, KdmPredicate::LinkId(), "volatile");
  writeTripleContains(KdmElementId_CxxExtensionFamily, KdmElementId_VolatileStereotype);

  writeTriple(KdmElementId_ConstStereotype, KdmPredicate::KdmType(), KdmType::Stereotype());
  writeTriple(KdmElementId_ConstStereotype, KdmPredicate::Name(), "const");
  writeTriple(KdmElementId_ConstStereotype, KdmPredicate::LinkId(), "const");
  writeTripleContains(KdmElementId_CxxExtensionFamily, KdmElementId_ConstStereotype);

  writeTriple(KdmElementId_StaticStereotype, KdmPredicate::KdmType(), KdmType::Stereotype());
  writeTriple(KdmElementId_StaticStereotype, KdmPredicate::Name(), "static");
  writeTriple(KdmElementId_StaticStereotype, KdmPredicate::LinkId(), "static");
  writeTripleContains(KdmElementId_CxxExtensionFamily, KdmElementId_StaticStereotype);

  writeTriple(KdmElementId_InlineStereotype, KdmPredicate::KdmType(), KdmType::Stereotype());
  writeTriple(KdmElementId_InlineStereotype, KdmPredicate::Name(), "inline");
  writeTriple(KdmElementId_InlineStereotype, KdmPredicate::LinkId(), "inline");
  writeTripleContains(KdmElementId_CxxExtensionFamily, KdmElementId_InlineStereotype);

  writeTriple(KdmElementId_RestrictStereotype, KdmPredicate::KdmType(), KdmType::Stereotype());
  writeTriple(KdmElementId_RestrictStereotype, KdmPredicate::Name(), "restrict");
  writeTriple(KdmElementId_RestrictStereotype, KdmPredicate::LinkId(), "restrict");
  writeTripleContains(KdmElementId_CxxExtensionFamily, KdmElementId_RestrictStereotype);

  writeTriple(KdmElementId_VirtualStereotype, KdmPredicate::KdmType(), KdmType::Stereotype());
  writeTriple(KdmElementId_VirtualStereotype, KdmPredicate::Name(), "virtual");
  writeTriple(KdmElementId_VirtualStereotype, KdmPredicate::LinkId(), "virtual");
  writeTripleContains(KdmElementId_CxxExtensionFamily, KdmElementId_VirtualStereotype);

  writeTriple(KdmElementId_PureVirtualStereotype, KdmPredicate::KdmType(), KdmType::Stereotype());
  writeTriple(KdmElementId_PureVirtualStereotype, KdmPredicate::Name(), "pure virtual");
  writeTriple(KdmElementId_PureVirtualStereotype, KdmPredicate::LinkId(), "pure virtual");
  writeTripleContains(KdmElementId_CxxExtensionFamily, KdmElementId_PureVirtualStereotype);

  writeTriple(KdmElementId_AbstractStereotype, KdmPredicate::KdmType(), KdmType::Stereotype());
  writeTriple(KdmElementId_AbstractStereotype, KdmPredicate::Name(), "abstract");
  writeTriple(KdmElementId_AbstractStereotype, KdmPredicate::LinkId(), "abstract");
  writeTripleContains(KdmElementId_CxxExtensionFamily, KdmElementId_AbstractStereotype);

  writeTriple(KdmElementId_FriendStereotype, KdmPredicate::KdmType(), KdmType::Stereotype());
  writeTriple(KdmElementId_FriendStereotype, KdmPredicate::Name(), "friend");
  writeTriple(KdmElementId_FriendStereotype, KdmPredicate::LinkId(), "friend");
  writeTripleContains(KdmElementId_CxxExtensionFamily, KdmElementId_FriendStereotype);

  writeTriple(KdmElementId_IncompleteStereotype, KdmPredicate::KdmType(), KdmType::Stereotype());
  writeTriple(KdmElementId_IncompleteStereotype, KdmPredicate::Name(), "incomplete");
  writeTriple(KdmElementId_IncompleteStereotype, KdmPredicate::LinkId(), "incomplete");
  writeTripleContains(KdmElementId_CxxExtensionFamily, KdmElementId_IncompleteStereotype);

  writeTriple(KdmElementId_PublicStereotype, KdmPredicate::KdmType(), KdmType::Stereotype());
  writeTriple(KdmElementId_PublicStereotype, KdmPredicate::Name(), "public");
  writeTriple(KdmElementId_PublicStereotype, KdmPredicate::LinkId(), "public");
  writeTripleContains(KdmElementId_CxxExtensionFamily, KdmElementId_PublicStereotype);

  writeTriple(KdmElementId_PrivateStereotype, KdmPredicate::KdmType(), KdmType::Stereotype());
  writeTriple(KdmElementId_PrivateStereotype, KdmPredicate::Name(), "private");
  writeTriple(KdmElementId_PrivateStereotype, KdmPredicate::LinkId(), "private");
  writeTripleContains(KdmElementId_CxxExtensionFamily, KdmElementId_PrivateStereotype);

  writeTriple(KdmElementId_ProtectedStereotype, KdmPredicate::KdmType(), KdmType::Stereotype());
  writeTriple(KdmElementId_ProtectedStereotype, KdmPredicate::Name(), "protected");
  writeTriple(KdmElementId_ProtectedStereotype, KdmPredicate::LinkId(), "protected");
  writeTripleContains(KdmElementId_CxxExtensionFamily, KdmElementId_ProtectedStereotype);

  writeTriple(KdmElementId_ExplicitStereotype, KdmPredicate::KdmType(), KdmType::Stereotype());
  writeTriple(KdmElementId_ExplicitStereotype, KdmPredicate::Name(), "explicit");
  writeTriple(KdmElementId_ExplicitStereotype, KdmPredicate::LinkId(), "explicit");
  writeTripleContains(KdmElementId_CxxExtensionFamily, KdmElementId_ExplicitStereotype);

  // Code Model contents
  writeTriple(KdmElementId_CodeAssembly, KdmPredicate::KdmType(), KdmType::CodeAssembly());
  writeTriple(KdmElementId_CodeAssembly, KdmPredicate::Name(), ":code");
  writeTriple(KdmElementId_CodeAssembly, KdmPredicate::LinkId(), ":code");
  writeTripleContains(KdmElementId_CodeModel, KdmElementId_CodeAssembly);
  writeTriple(KdmElementId_LanguageUnit, KdmPredicate::KdmType(), KdmType::LanguageUnit());
  writeTriple(KdmElementId_LanguageUnit, KdmPredicate::Name(), ":language");
  writeTriple(KdmElementId_LanguageUnit, KdmPredicate::LinkId(), ":language");
  writeTripleContains(KdmElementId_CodeAssembly, KdmElementId_LanguageUnit);
  writeTriple(KdmElementId_DerivedSharedUnit, KdmPredicate::KdmType(), KdmType::SharedUnit());
  writeTriple(KdmElementId_DerivedSharedUnit, KdmPredicate::Name(), ":derived");
  writeTriple(KdmElementId_DerivedSharedUnit, KdmPredicate::LinkId(), ":derived");
  writeTripleContains(KdmElementId_CodeAssembly, KdmElementId_DerivedSharedUnit);
  writeTriple(KdmElementId_ClassSharedUnit, KdmPredicate::KdmType(), KdmType::SharedUnit());
  writeTriple(KdmElementId_ClassSharedUnit, KdmPredicate::Name(), ":class");
  writeTriple(KdmElementId_ClassSharedUnit, KdmPredicate::LinkId(), ":class");
  writeTripleContains(KdmElementId_CodeAssembly, KdmElementId_ClassSharedUnit);
  writeTriple(KdmElementId_InventoryModel, KdmPredicate::KdmType(), KdmType::InventoryModel());
  writeTriple(KdmElementId_InventoryModel, KdmPredicate::Name(), KdmType::InventoryModel());
  writeTriple(KdmElementId_InventoryModel, KdmPredicate::LinkId(), KdmType::InventoryModel());
  writeTripleContains(KdmElementId_Segment,KdmElementId_InventoryModel);
  writeTripleKdmType(KdmElementId_CompilationUnit, KdmType::CompilationUnit());
  writeTripleName(KdmElementId_CompilationUnit, mCompilationFile.filename());
  writeTriple(KdmElementId_CompilationUnit, KdmPredicate::LinkId(), mCompilationFile.string());
  writeTripleContains(KdmElementId_CodeAssembly, KdmElementId_CompilationUnit);
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
  //add the node(s) to the uid graph
  bool result = updateUidGraph(parent, child);

  if (result)
  {
    //write contains relationship to file
    writeTriple(parent, KdmPredicate::Contains(), child);
  }
}

/**
 *
 */
long KdmTripleWriter::writeTripleExtends(long const subclass, long const superclass)
{
  long extendsId = ++mKdmElementId;
  writeTripleKdmType(extendsId, KdmType::Extends());
  writeTriple(extendsId, KdmPredicate::From(), subclass);
  writeTriple(extendsId, KdmPredicate::To(), superclass);
  return extendsId;
}

/**
 *
 */
long KdmTripleWriter::writeTripleFriend(long const subclass, long const superclass)
{
  long extendsId = ++mKdmElementId;
  writeTripleKdmType(extendsId, KdmType::CodeRelationship());
  writeTriple(extendsId, KdmPredicate::From(), subclass);
  writeTriple(extendsId, KdmPredicate::To(), superclass);
  writeTriple(extendsId, KdmPredicate::Stereotype(), KdmElementId_FriendStereotype);
  return extendsId;
}

bool KdmTripleWriter::updateUidGraph(long const parent, long const child)
{
  //We check to see if the parent or the child already has a node
  //in the graph since the creation of subtrees can happen in
  //any order
  boost::graph_traits<UidGraph>::vertex_iterator i, end;
  bool foundParentFlag = false;
  bool foundChildFlag = false;
  Vertex parentVertex = boost::graph_traits<UidGraph>::null_vertex();
  Vertex childVertex = boost::graph_traits<UidGraph>::null_vertex();

  for (boost::tie(i, end) = boost::vertices(mUidGraph); i != end; ++i)
  {
    if (parent == mUidGraph[*i].elementId)
    {
      foundParentFlag = true;
      parentVertex = *i;
    }
    if (child == mUidGraph[*i].elementId)
    {
      foundChildFlag = true;
      childVertex = *i;
    }
    if (foundParentFlag and foundChildFlag)
    {
      break;
    }
  }



  //If we don't have the parent in graph already create a new vertex otherwise
  //use the one we found
  if (not foundParentFlag)
  {
    parentVertex = boost::add_vertex(mUidGraph);
    mUidGraph[parentVertex].elementId = parent;
  }

  if (not foundChildFlag)
  {
    //Create our child vertex
    childVertex = boost::add_vertex(mUidGraph);
    mUidGraph[childVertex].elementId = child;
  }

  bool retVal = true;
  if (mSettings.containmentCheck)
  {
    ContainmentMap::const_iterator ci = mContainment.find(child);
    if (ci != mContainment.end())
    {
      std::string msg(str(boost::format("FIXME: %1% : Double containment element <%2%> is contained in <%3%> and <%4%>. %5%:%6%") % mCompilationFile % child % ci->second % parent % BOOST_CURRENT_FUNCTION % __LINE__ ));
      writeComment(msg);
      retVal = false;
    }
    else
    {
      mContainment.insert(std::make_pair(child, parent));
    }
  }

  if (retVal)
  {
    //create the edge between them
    boost::add_edge(parentVertex, childVertex, mUidGraph);
  }
  return retVal;
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

/**
 * Write the memberUnit and any associated data.
 */
long KdmTripleWriter::writeKdmMemberUnit(tree const member)
{
  long memberId = getReferenceId(member);
  writeTripleKdmType(memberId, KdmType::MemberUnit());
  tree type(TYPE_MAIN_VARIANT(TREE_TYPE(member)));
  long ref = getReferenceId(type);
  std::string name(nodeName(member));

  writeTripleName(memberId, name);
  writeTriple(memberId, KdmPredicate::Type(), ref);
  writeKdmSourceRef(memberId, member);

  // Set the export kind, if available
  tree context = CP_DECL_CONTEXT (member);
  if (TYPE_P(context))
  {
    if (TREE_PRIVATE (member)) writeTripleExport(memberId, "private");
    else if (TREE_PROTECTED (member)) writeTripleExport(memberId, "protected");
    else writeTripleExport(memberId, "public");
  }
  return memberId;
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
  tree type = TYPE_MAIN_VARIANT(TREE_TYPE(val));
  long ref = getReferenceId(type);

  writeTripleKdmType(valueId, KdmType::Value());
  writeTripleName(valueId, nodeName(val));
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


long KdmTripleWriter::getSourceFileReferenceId(tree const node)
{
  //The shared or compliation unit id
  long unitId(0);

  //Find the file the given node is located in ensure that is is complete
  expanded_location loc(expand_location(locationOf(node)));
  Path file(loc.file);
  if (!file.is_complete())
  {
    file = boost::filesystem::complete(file);
  }

  // the node is not in our translation unit, it must be in a shared unit
  if (mCompilationFile != file)
  {
    //Retrieve the identifier node for the file containing the given node
    tree identifierNode = get_identifier(loc.file);
    unitId = getSharedUnitReferenceId(identifierNode);
  }
  else
  {
    //the node is located in the translation unit; return the id for the translation unit
    //unless it's external in which case we don't know where it is
    //in which case we put it in the derivedSharedUnit by default
    if (DECL_P(node))
    {
      if (!DECL_EXTERNAL(node))
      {
        unitId = KdmElementId_CompilationUnit;
      }
      else
      {
        unitId = KdmElementId_DerivedSharedUnit;
      }
    }
    else
    {
      unitId = KdmElementId_CompilationUnit;
    }
  }
  return unitId;
}


long KdmTripleWriter::getUserTypeId(KdmType const & type)
{
  TypeMap::const_iterator i = mUserTypes.find(type);
  long retVal;
  if (i == mUserTypes.end())
  {
    long typeId = getNextElementId();
    mUserTypes.insert(std::make_pair(type,typeId));

    writeTripleKdmType(typeId, type);
    writeTripleName(typeId, type.name());
    writeTripleContains(KdmElementId_LanguageUnit, typeId);
    retVal = typeId;
  }
  else
  {
    retVal = i->second;
  }
  return retVal;
}


long KdmTripleWriter::getSharedUnitReferenceId(tree const identifierNode)
{
  long retValue(-1);

  //Attempt insertion with an artificial element id;
  std::pair<TreeMap::iterator, bool> result = mReferencedSharedUnits.insert(std::make_pair(identifierNode, mKdmElementId + 1));

  //We haven't seen that identifier before, increment the element id and return it
  if (result.second)
  {
    retValue = ++mKdmElementId;
    writeKdmSharedUnit(identifierNode);
  }
  // We have already encountered that identifier return the id for it
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

  //for (tree tmp = arrayType; TREE_CODE (tmp) == ARRAY_TYPE; tmp = TREE_TYPE (tmp))
//    dump_array_domain (buffer, TYPE_DOMAIN (tmp), spc, flags);
    tree domain = TYPE_DOMAIN(arrayType);
    if (domain)
    {
      tree min = TYPE_MIN_VALUE (domain);
      tree max = TYPE_MAX_VALUE (domain);
      if (min && max && integer_zerop (min) && host_integerp (max, 0))
      {
        std::string nameStr = boost::str(boost::format(HOST_WIDE_INT_PRINT_DEC) % (TREE_INT_CST_LOW (max) + 1));
        writeTriple(arrayKdmElementId, KdmPredicate::Size(), nameStr);
      }
      else
      {
        if (min)
        {
          writeTriple(arrayKdmElementId, KdmPredicate::Size(), nodeName(min));
        }
        if (max)
        {
          //dump the node?
          processAstNode(max);
          std::string msg(str(boost::format("writeKdmArrayType unsupported size: %1%:%2%") % BOOST_CURRENT_FUNCTION % __LINE__ ));
          writeUnsupportedComment(msg);
        }
      }
    }
    else
    {
      writeTriple(arrayKdmElementId, KdmPredicate::Size(), "<unknown>");
    }

}

/**
 * Record Types all need to output the following standard information:
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
    writeKdmClassType(recordType);
  }
  else //Record or Union
  {
    long compilationUnitId(getSourceFileReferenceId(mainRecordType));

    //struct
    long structId = getReferenceId(mainRecordType);
    writeTripleKdmType(structId, KdmType::RecordType());
    std::string name;
    //check to see if we are an anonymous struct
    name = (isAnonymousStruct(mainRecordType)) ? unnamedNode : nodeName(mainRecordType);
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

/**
 *
 */
void KdmTripleWriter::writeKdmTripleFriends(long const id, tree const befriending)
{
  for (tree frnd = befriending; frnd; frnd = TREE_CHAIN (frnd))
  {
    if(TREE_CODE (TREE_VALUE (frnd)) != TEMPLATE_DECL)
    {
      if(TREE_CODE (TREE_VALUE (frnd)) != TEMPLATE_DECL)
      {
        tree friendType TREE_VALUE (frnd);
        long friendId = getReferenceId(friendType);
        long relId = writeTripleFriend(id, friendId);
        writeTripleContains(id, relId);
      }
    }
  }
}

/**
 *
 */
void KdmTripleWriter::writeKdmClassType(tree const recordType)
{
  tree mainRecordType = TYPE_MAIN_VARIANT (recordType);
  long compilationUnitId(getSourceFileReferenceId(mainRecordType));
  //class

  long classId = getReferenceId(mainRecordType);
  writeTripleKdmType(classId, KdmType::ClassUnit());
  std::string name;
  //check to see if we are an anonymous class
  name = (isAnonymousStruct(mainRecordType)) ? unnamedNode : nodeName(mainRecordType);
  writeTripleName(classId, name);

  // link:id is the mangled name, hopefully
  writeTripleLinkId(classId, gcckdm::getLinkId(TYPE_NAME(recordType), name));

  // Is this an abstract class?
  if (CLASSTYPE_PURE_VIRTUALS (recordType) != 0)
    writeTriple(classId, KdmPredicate::Stereotype(), KdmElementId_AbstractStereotype);

  if (!COMPLETE_TYPE_P (recordType))
    writeTriple(classId, KdmPredicate::Stereotype(), KdmElementId_IncompleteStereotype);

  // Hide artificial classes
  if (DECL_ARTIFICIAL (recordType))
    writeTriple(classId, KdmPredicate::Stereotype(), KdmElementId_HiddenStereotype);

  // Base class information
  // See http://codesynthesis.com/~boris/blog/2010/05/17/parsing-cxx-with-gcc-plugin-part-3/
  tree biv (TYPE_BINFO (recordType));
  size_t n (biv ? BINFO_N_BASE_BINFOS (biv) : 0);

  for (size_t i (0); i < n; i++)
  {
    tree bi (BINFO_BASE_BINFO (biv, i));

    tree b_type (TYPE_MAIN_VARIANT (BINFO_TYPE (bi)));
    //    tree b_decl (TYPE_NAME (b_type));
    //    tree b_id (DECL_NAME (b_decl));

    long superClassId = getReferenceId(b_type);

    long extendsId = writeTripleExtends(classId, superClassId);

    // Add some Stereotypes to the relationship
    // Virtual subclass?
    bool virt (BINFO_VIRTUAL_P (bi));
    if(virt)
    {
      writeTriple(extendsId, KdmPredicate::Stereotype(), KdmElementId_VirtualStereotype);
    }

    // Access specifier.
    if (BINFO_BASE_ACCESSES (biv))
    {
      tree ac (BINFO_BASE_ACCESS (biv, i));

      if (ac == 0 || ac == access_public_node)
        writeTriple(extendsId, KdmPredicate::Stereotype(), KdmElementId_PublicStereotype);
      else if (ac == access_protected_node)
        writeTriple(extendsId, KdmPredicate::Stereotype(), KdmElementId_ProtectedStereotype);
      else
        writeTriple(extendsId, KdmPredicate::Stereotype(), KdmElementId_PrivateStereotype);

    }
    else writeTriple(extendsId, KdmPredicate::Stereotype(), KdmElementId_PublicStereotype);

    // Write the containment
    writeTripleContains(classId, extendsId);
  }

  // Friends
  writeKdmTripleFriends(classId, CLASSTYPE_BEFRIENDING_CLASSES(recordType));

  // Traverse members.
  for (tree d (TYPE_FIELDS (mainRecordType)); d != 0; d = TREE_CHAIN (d))
  {
    switch (TREE_CODE (d))
    {
      case TYPE_DECL:
      {
        if (!DECL_SELF_REFERENCE_P (d))
        {
          processAstNode(d);
        }
        break;
      }
      case FIELD_DECL:
      {
        if (!DECL_ARTIFICIAL (d))
        {
          long itemId = getReferenceId(d);
          processAstNode(d);
          writeTripleContains(classId, itemId);
        }
        break;
      }
      default:
      {
        processAstNode(d);
        break;
      }
    }
  }

  for (tree d (TYPE_METHODS (mainRecordType)); d != 0; d = TREE_CHAIN (d))
  {
    if (!DECL_ARTIFICIAL (d))
    {
      processAstNode(d);
    }
  }

  writeKdmSourceRef(classId, mainRecordType);
  writeTripleContains(compilationUnitId, classId);

}

void KdmTripleWriter::writeKdmSharedUnit(tree const file)
{
  long id = getSharedUnitReferenceId(file);
  Path filename(IDENTIFIER_POINTER(file));
  writeKdmSharedUnit(filename, id);
}

void KdmTripleWriter::writeKdmSharedUnit(Path const & file, const long id)
{
  writeTripleKdmType(id, KdmType::SharedUnit());
  writeTripleName(id, file.filename());
  writeTripleLinkId(id, file.string());
  writeTripleContains(KdmElementId_CodeAssembly, id);
}

long KdmTripleWriter::writeKdmSourceRef(long id, const tree node)
{
  return writeKdmSourceRef(id, expand_location(locationOf(node)));
}

long KdmTripleWriter::writeKdmSourceRef(long id, const expanded_location & eloc)
{
  if(!eloc.file)
  {
    return id;
  }
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
