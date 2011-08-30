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
//
// libGccKdm is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with libGccKdm.  If not, see <http://www.gnu.org/licenses/>.
//

#include "gcckdm/kdmtriplewriter/KdmTripleWriter.hh"

#include <algorithm>
#include <iterator>

#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/current_function.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include "gcckdm/GccKdmConfig.hh"
#include "gcckdm/KdmPredicate.hh"
#include "gcckdm/kdmtriplewriter/GimpleKdmTripleWriter.hh"
#include "gcckdm/kdmtriplewriter/Exception.hh"
#include "gcckdm/GccKdmVersion.hh"
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/graphviz.hpp>

#include <fstream>

namespace
{

namespace ktw = gcckdm::kdmtriplewriter;

/// Prefix for all KDM triple format comments
std::string const commentPrefix("# ");

/// Prefix for all KDM triple unsupported node statements, used in conjunction with commentPrefix
std::string const unsupportedPrefix("UNSUPPORTED: ");

std::string const unnamedStructNode("<unnamed struct>");
std::string const unnamedClassNode("<unnamed class>");

std::string const linkCallablePrefix("c.function/");

std::string const linkVariablePrefix("c.variable/");

long unnamedStructNumber = 1;

/**
 * True if the given AST node is an anonymous struct
 *
 * @param t the node to test
 * @return true if t is an anonymous struct
 */
bool isAnonymousStruct(tree const t)
{
  tree name = TYPE_NAME (t);
  if (name && TREE_CODE (name) == TYPE_DECL)
  {
    name = DECL_NAME (name);
  }
  return !name || ANON_AGGRNAME_P (name);
}

void fixPathString(std::string & pathStr)
{
  //Gcc likes to put double slashes at the start of the string... boost::filesystem
  //doesn't like them very much and messes up iteration... so we remove them
  boost::replace_all(pathStr, "//", "/");

  //Correct windows paths so boost::filesystem doesn't bork
  boost::replace_all(pathStr, "\\", "/");
}

/**
 * If the outputCompletePath setting is true, the given path is completed and normalized and tested for
 * existance, if it exists the complete and normalized path is returned, otherwise the original
 * path is returned unaltered.
 *
 * @param settings the user defined compile time settings
 * @param p the path to attempt to complete and normalize
 */
ktw::KdmTripleWriter::Path checkPathType(ktw::KdmTripleWriter::Settings const & settings, ktw::KdmTripleWriter::Path const & p)
{
  ktw::KdmTripleWriter::Path tmp = p;
  if (settings.outputCompletePath)
  {
    tmp = boost::filesystem::complete(p);
  }
  tmp.normalize();
  return boost::filesystem::exists(tmp) ? tmp : p;
}


std::string getLinkIdForType(tree type)
{
  return gcckdm::locationString(gcckdm::locationOf(type));
}

} // namespace


namespace gcckdm
{

namespace kdmtriplewriter
{

namespace bfs = boost::filesystem;

/**
 * This class is used when writing the contains graph via graphviz
 */
class VertexPropertyWriter
{
public:
  typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, UidNode> Graph;

  VertexPropertyWriter(Graph& graph) :
    mGraph(graph)
  {
  }

  /**
   * Ensure that label is the elementId instead of a random number
   */
  template<class Vertex>
  void operator()(std::ostream& out, const Vertex v) const
  {
    out << "[label=\"" << mGraph[v].elementId << "\"]";
  }

private:
  Graph & mGraph;
};

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
class KdmTripleWriter::UidVisitor: public boost::default_dfs_visitor
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
  UidVisitor(KdmTripleWriter & writer, UidGraph & graph) :
    mWriter(writer),
    mGraph(graph)
  {
  }

  /**
   * Called when a new vertex is discovered in the DFS traversal
   */
  void discover_vertex(KdmTripleWriter::Vertex v, KdmTripleWriter::UidGraph const & g)
  {
    //KdmTripleWriter::UidGraph & gg= const_cast<KdmTripleWriter::UidGraph&>(g);
    //std::cerr << "Start:" << g[v].elementId << std::endl;
    if (mGraph[v].incrementFlag)
    {
      mGraph[v].startUid = ++mWriter.mUid;
    }
    else
    {
      mGraph[v].startUid = mWriter.mUid;
    }
    mLastVertex = v;
  }

  /**
   * Called when we are leaving a vertex in the DFS traversal
   */
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
    if (mGraph[v].startUid != mGraph[v].endUid)
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

/**
 * Constructs the triple writing dumping output to the given sink.
 */
KdmTripleWriter::KdmTripleWriter(KdmSinkPtr const & kdmSinkPtr, KdmTripleWriter::Settings const & settings) :
  mKdmSink(kdmSinkPtr),
  mKdmElementId(KdmElementId_DefaultStart),
  mReferencedNodes(),
  mReferencedSharedUnits(),
  mCompilationFile(),
  mInventoryMap(),
  mProcessedNodes(),
  mNodeQueue(),
#if 0 //BBBB
  mValues(),
#endif
  mUidGraph(),
  mUid(0),
  mUserTypes(),
  mContainment(),
  mItemUnits(),
  mSettings(settings),
  mPackageMap(),
  mDirectoryMap(),
  mLockUid(false)
{
  //We pass this object to the gimple writer to allow it to use our triple writing powers
  mGimpleWriter.reset(new GimpleKdmTripleWriter(*this, settings));
}

/**
 * This constructor will dump it's output to the given filename,
 * we use the settings output directory to modify the location of the output filename
 *
 */
KdmTripleWriter::KdmTripleWriter(Path const & filename, KdmTripleWriter::Settings const & settings) :
  mKdmElementId(KdmElementId_DefaultStart),
  mReferencedNodes(),
  mReferencedSharedUnits(),
  mCompilationFile(),
  mInventoryMap(),
  mProcessedNodes(),
  mNodeQueue(),
#if 0 //BBBB
  mValues(),
#endif
  mUidGraph(),
  mUid(0),
  mUserTypes(),
  mContainment(),
  mItemUnits(),
  mSettings(settings),
  mPackageMap(),
  mDirectoryMap(),
  mLockUid(false)
{
  if (settings.outputDir.filename().empty())
  {
    mKdmSink.reset(new bfs::ofstream(filename));
  }
  else
  {
    mKdmSink.reset(new bfs::ofstream(mSettings.outputDir / filename.filename()));
  }

  //We pass this object to the gimple writer to allow it to use our triple writing powers
  mGimpleWriter.reset(new GimpleKdmTripleWriter(*this, settings));
}

/**
 * Ensure that the kdmSink is flushed before destruction
 */
KdmTripleWriter::~KdmTripleWriter()
{
  //Write out anything remaining in the stream
  mKdmSink->flush();
}

/**
 * yay, we are starting!  We are given the file being compiled
 * the current translation unit.  Since it's easier to always work
 * with absolute files we ensure the file is absoloute.
 *
 * Write out the version of the output, write our standard common
 * KDM elements
 */
void KdmTripleWriter::startTranslationUnit(Path const & file)
{
  try
  {
    mCompilationFile = file;
    writeVersionHeader();
    writeDefaultKdmModelElements();
  }
  catch (KdmTripleWriterException & e)
  {
    std::cerr << boost::diagnostic_information(e);
  }
}

/**
 * For C, we have to traverse all variable stored in the var pool
 */
void KdmTripleWriter::startKdmGimplePass()
{
  try
  {
    //C Support..variables are stored in varpool... C++ we can use global_namespace?
    struct varpool_node *pNode;
    for ((pNode) = varpool_nodes_queue; (pNode); (pNode) = (pNode)->next_needed)
    {
      if (!hasReferenceId(pNode->decl))
      {
        writeKdmStorableUnit(pNode->decl, WriteKdmContainsRelation);
        writeKdmStorableUnitKindGlobal(pNode->decl);
        //Since we are writing storable units directly and not using the processAstXXXX methods
        //we have to manually mark the decl as processed to ensure that it isn't processed
        //elsewhere
        markNodeAsProcessed(pNode->decl);
      }
    }
  }
  catch (KdmTripleWriterException & e)
  {
    std::cerr << boost::diagnostic_information(e);
  }
}

/**
 * Nothing much to do here except ensure that any nodes that have been
 * placed on the queue while processing other nodes are processsed.
 */
void KdmTripleWriter::finishKdmGimplePass()
{
  try
  {
    processNodeQueue();
  }
  catch (KdmTripleWriterException & e)
  {
    std::cerr << boost::diagnostic_information(e);
  }
}

void KdmTripleWriter::processNodeQueue()
{
  //Process any nodes that are still left on the queue
  for (; !mNodeQueue.empty(); mNodeQueue.pop())
  {
    tree node = mNodeQueue.front();

#if 0 //BBBB TMP
    if ((const unsigned long)node == 0xb7d8aae0) {
    	int junk = 123;
    }
#endif

    //Keeping this here for debugging
    //    fprintf(stderr,"mNodeQueue.front: %p <%ld>\n", node, getReferenceId(node));
    processAstNode(node);
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
  try
  {
    //process any remaining nodes
    processNodeQueue();

    //finally If the user wants UIDs in the output,
    //we write them here
    if (mSettings.generateUids)
    {
      writeUids();
    }
  }
  catch (KdmTripleWriterException & e)
  {
    std::cerr << boost::diagnostic_information(e);
  }
}


//Simply call the internal version of process node
void KdmTripleWriter::processAstNode(tree const ast)
{
  processAstNodeInternal(ast);
}


long KdmTripleWriter::processAstNodeInternal(tree const ast, ContainsRelationPolicy const containPolicy, bool isTemplate)
{
  long id = invalidId;
  try
  {
    //    good debug code
    //    if (hasReferenceId(ast))
    //    {
    //      int i = getReferenceId(ast);
    //      std::cerr << "refid: " << i << std::endl;
    //    }

    //Ensure we haven't processed this node before
    if (mProcessedNodes.find(ast) == mProcessedNodes.end())
    {
#if 0 //BBBB-4444
      markNodeAsProcessed(ast);
#endif
#if 0 //BBBB TMP
      if ((const unsigned long)ast == 0xb7b653c8)
      {
        int junk = 123;
      }
#endif
      int treeCode(TREE_CODE(ast));

      //All non built-in delcarations go here...
      if (DECL_P(ast))
      {

#if 0 //BBBB
        if (DECL_IS_BUILTIN(ast))
        {
          // SKIP all built-in declarations??
        }
        else
#endif
        {
          id = processAstDeclarationNode(ast, containPolicy, isTemplate);
        }
      }
      else if (treeCode == NAMESPACE_DECL)
      {
        processAstNamespaceNode(ast);
      }
      else if (treeCode == LABEL_DECL)
      {
        //Not required.. . labels are handled in the gimple
      }
      else if (treeCode == TREE_LIST)
      {
        // FIXME: Is this the correct handling for the tree_list?
        tree l = ast;
        while (l)
        {
          //          tree treePurpose = TREE_PURPOSE(l);
          tree treeValue = TREE_VALUE(l);
          if (treeValue)
          {
            processAstNodeInternal(treeValue);
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
        // GCC In all it's glory can have different tree nodes represent the same type
        // The TYPE_MAIN_VARIANT macro returns the primary, cvr-unqualified type from
        // which all the cvr-qualified and other copies have been created.. whew..
        // I wish I really knew what that meant.  The gist is that for types we cannot
        // store the regular ast node in the processedNodes map we have to use the
        // TYPE_MAIN_VARIANT to get the proper mode
        id = processAstTypeNode(ast);
      }
      else
      {
        std::string msg(str(boost::format("<%3%> AST Node (%1%) in %2%:%4%") % tree_code_name[treeCode] % BOOST_CURRENT_FUNCTION % getReferenceId(ast) % __LINE__));
        writeUnsupportedComment(msg);
      }

      {
        markNodeAsProcessed(ast);
      }
    }
    else
    {
      id = getReferenceId(ast);
    }
  }
  catch (KdmTripleWriterException & e)
  {
    std::cerr << boost::diagnostic_information(e);
  }
  return id;
}


long KdmTripleWriter::processAstDeclarationNode(tree const decl, ContainsRelationPolicy const containPolicy, bool isTemplate)
{
#if 0 //BBBB-TMP-DBG
  long junk = getReferenceId(decl);
#endif

  long id = invalidId;

  assert(DECL_P(decl));

  int treeCode = TREE_CODE(decl);
  switch (treeCode)
  {
    case VAR_DECL:
    {
      processAstVariableDeclarationNode(decl);
      break;
    }
    case FUNCTION_DECL:
    {
      processAstFunctionDeclarationNode(decl, containPolicy, isTemplate);
      break;
    }
    case FIELD_DECL:
    {
      processAstFieldDeclarationNode(decl);
      break;
    }
    case PARM_DECL:
    {
#if 0 //BBBB
      writeComment("FIXME: Do we need these parm_decls?");
#endif
      return id;
    }
    case TYPE_DECL:
    {
      id = processAstTypeDecl(decl);
      break;
    }
    case NAMESPACE_DECL:
    {
      //For the moment we ignore these.
      return id;
    }
    case CONST_DECL:
    {
      // For the moment we ignore these. Gimple seems to sub in the actual value anyway.
      break;
    }
    case TEMPLATE_DECL:
    {
      processAstTemplateDecl(decl);
      break;
    }
      //    case RESULT_DECL:
      //    {
      //
      //      //Should this be moved to gimple?
      //      writeKdmStorableUnit(decl, WriteKdmContainsRelation, LocalStorableUnitScope);
      //      //Ignore for the moment.  In tree-pretty-print.c they simply print "<retval>"
      //      //It's possible that we should do something with this but for now.....
      //      //Found in gvariant-serialiser.tkdm - glib-2.24.2
      //      //Found in gtktext.kdm = gtk+-2.22.0
      //      break;
      //    }
    case LABEL_DECL:
    {
#if 0 //BBBB
      writeComment("FIXME: We are skipping a label_decl here is it needed?");
#endif
      //      processAstLabelDeclarationNode(decl);
      return id;
    }
    default:
    {
      std::string msg(str(boost::format("AST Declaration Node (%1%) in %2%") % tree_code_name[treeCode] % BOOST_CURRENT_FUNCTION));
      writeUnsupportedComment(msg);
      return id;
    }
  }

  writeKdmTypeQualifiers(decl);

  return id;
}


/**
 *
 */
void KdmTripleWriter::writeKdmTypeQualifiers(tree const decl)
{
  long id = getReferenceId(decl);
  // FIXME: This code was intended to detect constant functions. It does not currently work
  if (DECL_NONSTATIC_MEMBER_FUNCTION_P (decl))
  {
    tree treeType = TREE_TYPE (decl);
    tree typeArgsType = TYPE_ARG_TYPES (treeType);
    tree treeValue = TREE_VALUE (typeArgsType);
    tree treeValueType = TREE_TYPE (treeValue);

    int qualifiers = gcckdm::getTypeQualifiers(treeValueType);
    if (qualifiers & TYPE_QUAL_CONST)
    {
      writeTriple(id, KdmPredicate::Stereotype(), KdmElementId_ConstStereotype);
    }
  }

  // Type qualifiers: Assemble any applicable Stereotypes that supply additional information
  int qualifiers = gcckdm::getTypeQualifiers(decl);
  if (qualifiers & TYPE_QUAL_CONST)
  {
    writeTriple(id, KdmPredicate::Stereotype(), KdmElementId_ConstStereotype);
  }
  if (qualifiers & TYPE_QUAL_VOLATILE)
  {
    writeTriple(id, KdmPredicate::Stereotype(), KdmElementId_VolatileStereotype);
  }
  if (qualifiers & TYPE_QUAL_RESTRICT)
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
long KdmTripleWriter::processAstTypeDecl(tree const typeDecl)
{
  tree typeNode = TREE_TYPE (typeDecl);

  if (typeNode)
  {
    int treeCode = TREE_CODE(typeDecl);
    int typeCode = TREE_CODE(typeNode);
    if (treeCode == TYPE_DECL && typeCode == RECORD_TYPE)
    {
      // If this is artificial then it is a class declaration, otherwise a typedef.
      if (DECL_ARTIFICIAL(typeDecl))
      {

        /* BBBB: This should NOT happen (such nodes should NOT be put onto the queue.
         *       *BUT* it happens somehow with templates...*/
//        std::string msg(str(boost::format("DECL_ARTIFICIAL(%1%) in %2%") % tree_code_name[TREE_CODE (typeDecl)] % BOOST_CURRENT_FUNCTION));
//        writeUnsupportedComment(msg);

        return processAstNodeInternal(typeNode);
      }
    }
  }

  // Otherwise it is a typedef...
  long typedefKdmElementId = getReferenceId(typeDecl);
  writeTripleKdmType(typedefKdmElementId, KdmType::TypeUnit());

  // Get the name for the typedef, if available
  tree id = DECL_NAME (typeDecl);
  std::string name = nodeName(id);
  writeTripleName(typedefKdmElementId, nodeName(id));
  writeTripleLinkId(typedefKdmElementId, name);

  tree const typeMainVariant = TYPE_MAIN_VARIANT(typeNode);
  long typeKdmElementId = getReferenceId(typeMainVariant);
  writeTriple(typedefKdmElementId, KdmPredicate::Type(), typeKdmElementId);

  //Write the location of this typedef
  writeKdmSourceRef(typedefKdmElementId, typeDecl);

  // Write the containment information
  writeKdmCxxContains(typedefKdmElementId, typeDecl);

  return typedefKdmElementId;
}


/* Recursively search a type node for template parameters.  */
int KdmTripleWriter::find_template_parm(tree t)
{
  if (!t)
  {
    return 0;
  }

  switch (TREE_CODE (t))
  {
    /* A vector of template arguments on an instantiation.  */
    case TREE_VEC:
    {
      int i;
      for (i = 0; i < TREE_VEC_LENGTH (t); ++i)
      {
        if (find_template_parm(TREE_VEC_ELT (t, i)))
        {
          return 1;
        }
      }
      break;
    }
      /* A type list has nested types.  */
    case TREE_LIST:
    {
      if (find_template_parm(TREE_PURPOSE (t)))
      {
        return 1;
      }
      return find_template_parm(TREE_VALUE (t));
      break;
    }
      /* Template parameter types.  */
    case TEMPLATE_TYPE_PARM:
      return 1;
    case TEMPLATE_TEMPLATE_PARM:
      return 1;
    case TEMPLATE_PARM_INDEX:
      return 1;
    case TYPENAME_TYPE:
      return 1;
    case BOUND_TEMPLATE_TEMPLATE_PARM:
      return 1;

      /* A constant or variable declaration is encountered when a
       template instantiates another template using an enum or static
       const value that is not known until the outer template is
       instantiated.  */
    case CONST_DECL:
      return 1;
    case VAR_DECL:
      return 1;
    case FUNCTION_DECL:
      return 1;
    case FIELD_DECL:
      return 1;

      /* A template deferred scoped lookup.  */
    case SCOPE_REF:
      return 1;

      /* A cast of a dependent expression.  */
    case CAST_EXPR:
      return 1;

      /* Types with nested types.  */
    case METHOD_TYPE:
    case FUNCTION_TYPE:
    {
      tree arg_type = TYPE_ARG_TYPES (t);
      if (find_template_parm(TREE_TYPE (t)))
      {
        return 1;
      }
      while (arg_type && (arg_type != void_list_node))
      {
        if (find_template_parm(arg_type))
        {
          return 1;
        }
        arg_type = TREE_CHAIN (arg_type);
      }
    }
      break;
    case UNION_TYPE:
    case QUAL_UNION_TYPE:
    case RECORD_TYPE:
    {
      if ((TREE_CODE (t) == RECORD_TYPE) && TYPE_PTRMEMFUNC_P (t))
      {
        return find_template_parm(TYPE_PTRMEMFUNC_FN_TYPE (t));
      }
      if (CLASSTYPE_TEMPLATE_INFO (t))
      {
        return find_template_parm(CLASSTYPE_TI_ARGS (t));
      }
    }
    case REFERENCE_TYPE:
      return find_template_parm(TREE_TYPE (t));
    case POINTER_TYPE:
      return find_template_parm(TREE_TYPE (t));
    case ARRAY_TYPE:
      return find_template_parm(TREE_TYPE (t));
    case OFFSET_TYPE:
    {
      return (find_template_parm(TYPE_OFFSET_BASETYPE (t)) || find_template_parm(TREE_TYPE (t)));
    }
    case PTRMEM_CST:
    {
      return (find_template_parm(PTRMEM_CST_CLASS (t)) || find_template_parm(PTRMEM_CST_MEMBER(t)));
    }

      /* Fundamental types have no nested types.  */
    case BOOLEAN_TYPE:
      return 0;
    case COMPLEX_TYPE:
      return 0;
    case ENUMERAL_TYPE:
      return 0;
    case INTEGER_TYPE:
      return 0;
    case LANG_TYPE:
      return 0;
    case REAL_TYPE:
      return 0;
    case VOID_TYPE:
      return 0;

      /* Template declarations are part of instantiations of template
       template parameters.  */
    case TEMPLATE_DECL:
      return 0;

      /* Unary expressions.  */
    case ALIGNOF_EXPR:
    case SIZEOF_EXPR:
    case ADDR_EXPR:
    case CONVERT_EXPR:
    case NOP_EXPR:
    case NEGATE_EXPR:
    case BIT_NOT_EXPR:
    case TRUTH_NOT_EXPR:
    case PREDECREMENT_EXPR:
    case PREINCREMENT_EXPR:
    case POSTDECREMENT_EXPR:
    case POSTINCREMENT_EXPR:
      return find_template_parm(TREE_OPERAND (t, 0));

      /* Binary expressions.  */
    case COMPOUND_EXPR:
    case INIT_EXPR:
    case MODIFY_EXPR:
    case PLUS_EXPR:
    case POINTER_PLUS_EXPR:
    case MINUS_EXPR:
    case MULT_EXPR:
    case TRUNC_DIV_EXPR:
    case TRUNC_MOD_EXPR:
    case MIN_EXPR:
    case MAX_EXPR:
    case LSHIFT_EXPR:
    case RSHIFT_EXPR:
    case BIT_IOR_EXPR:
    case BIT_XOR_EXPR:
    case BIT_AND_EXPR:
    case TRUTH_ANDIF_EXPR:
    case TRUTH_ORIF_EXPR:
    case LT_EXPR:
    case LE_EXPR:
    case GT_EXPR:
    case GE_EXPR:
    case EQ_EXPR:
    case NE_EXPR:
    case UNGT_EXPR:
    case UNGE_EXPR:
    case UNLT_EXPR:
    case UNLE_EXPR:
    case UNEQ_EXPR:
    case LTGT_EXPR:
    case EXACT_DIV_EXPR:
      return (find_template_parm(TREE_OPERAND (t, 0)) ||
              find_template_parm(TREE_OPERAND (t, 1)));

      /* Ternary expressions.  */
    case COND_EXPR:
      return (find_template_parm(TREE_OPERAND (t, 0)) ||
              find_template_parm(TREE_OPERAND (t, 1)) ||
              find_template_parm(TREE_OPERAND (t, 2)));

      /* Other types that have no nested types.  */
    case INTEGER_CST:
      return 0;
    case STATIC_CAST_EXPR:
      return 0;
    default:
      std::string msg(str(boost::format("AST Node (%1%) in %2%") % tree_code_name[TREE_CODE (t)] % BOOST_CURRENT_FUNCTION));
      writeUnsupportedComment(msg);
  }
  return 0;
}


/**
 *
 */
void KdmTripleWriter::processAstTemplateDecl(tree const templateDecl)
{
  tree templateParms = DECL_TEMPLATE_PARMS (templateDecl);
  //Ensure we haven't processed this TemplateDecl before
  if (mProcessedNodes.find(templateParms) == mProcessedNodes.end())
  {
    long templateUnitId = getReferenceId(templateDecl);

    // Dump the template specializations.
    for (tree tl = DECL_TEMPLATE_SPECIALIZATIONS (templateDecl); tl; tl = TREE_CHAIN (tl))
    {
      tree ts = TREE_VALUE (tl);
      int treeCode = TREE_CODE (ts);
      switch (treeCode)
      {
        case FUNCTION_DECL:
        {
          writeComment("--- Start Specialization");
          processAstNodeInternal(ts);
          writeComment("--- End Specialization");
          break;
        }
        case TEMPLATE_DECL:
          break;
        default:
        {
#if 0
          std::string msg(str(boost::format("AST Template Specialization Node (%1%) in %2%") % tree_code_name[treeCode] % BOOST_CURRENT_FUNCTION));
          writeUnsupportedComment(msg);
#endif
          break;
        }
      }
    }

    /* Dump the template instantiations.  */
    for (tree tl = DECL_TEMPLATE_INSTANTIATIONS (templateDecl); tl; tl = TREE_CHAIN (tl))
    {
      tree ts = TYPE_NAME (TREE_VALUE (tl));
      int treeCode = TREE_CODE (ts);
      switch (treeCode)
      {
        case TYPE_DECL:
        {
          /* Add the instantiation only if it is real.  */
          if (!find_template_parm(TYPE_TI_ARGS(TREE_TYPE(ts))))
          {
            long instantiationTypeId = processAstNodeInternal(ts);
            writeRelation(KdmType::InstanceOf(), instantiationTypeId /*fromId*/, templateUnitId /*toId*/);
          }
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
    if (TREE_CODE (TREE_TYPE (templateDecl)) == RECORD_TYPE || TREE_CODE (TREE_TYPE (templateDecl)) == UNION_TYPE
        || TREE_CODE (TREE_TYPE (templateDecl)) == QUAL_UNION_TYPE)
    {
      for (tree tl = TYPE_FIELDS (TREE_TYPE (templateDecl)); tl; tl = TREE_CHAIN (tl))
      {
        int treeCode = TREE_CODE (tl);
        if (treeCode == TEMPLATE_DECL)
        {
          //        std::string msg(str(boost::format("AST Template Instantiation Node (%1%) in %2%") % tree_code_name[treeCode] % BOOST_CURRENT_FUNCTION));
          //        writeUnsupportedComment(msg);
          //        //        processAstNodeInternal(tl);

          //        processAstTemplateDecl(tl);

        }
      }
    }

    // Ensure we haven't processed this TemplateUnit before
    tree templateDeclType = TREE_TYPE (templateDecl);
    if (mProcessedNodes.find(templateDeclType) == mProcessedNodes.end())
    {
      //    long templateDeclTypeId = getReferenceId(templateDeclType);

      // Write TemplateUnit
      std::string name(nodeName(templateDecl));
      writeTripleKdmType(templateUnitId, KdmType::TemplateUnit());
      writeTripleName(templateUnitId, name);
	  writeTripleLinkId(templateUnitId, name);
      writeKdmSourceRef(templateUnitId, templateDecl);
      writeKdmCxxContains(templateUnitId, templateDecl);

      // Write TemplateParameters
      tree parms = DECL_TEMPLATE_PARMS (templateDecl);
#if 1  // We only want the first "layer" of parameters here, other layers are handled with their respective templates.
      tree t = parms;
      if (t)
      {
#else
        for (tree t = parms; t; t = TREE_CHAIN (t))
        {
#endif
        tree vparms = TREE_VALUE (t);
        int nparms = TREE_VEC_LENGTH (vparms);
        for (int parm_idx = 0; parm_idx < nparms; ++parm_idx)
        {
          tree tparm = TREE_VALUE (TREE_VEC_ELT (vparms, parm_idx));
          tree typeDecl = tparm;
          tree typeNode = TREE_TYPE (typeDecl);
          tree id(DECL_NAME (typeDecl));
          //          if (!id) {
          //            std::string msg(str(boost::format("AST Node (%1%) in %2%") % tree_code_name[TREE_CODE(tparm)] % BOOST_CURRENT_FUNCTION));
          //            writeUnsupportedComment(msg);
          //          }
          std::string typeDeclName = nodeName(id);
          long typeDeclId = getReferenceId(typeDecl);
          long typeId = getReferenceId(typeNode);
          markNodeAsProcessed(typeDecl);
          writeTripleKdmType(typeDeclId, KdmType::TemplateParameter());
          writeTripleName(typeDeclId, typeDeclName);
          writeTriple(typeDeclId, KdmPredicate::Type(), typeId);
          writeKdmSourceRef(typeDeclId, typeNode);
          writeTripleContains(templateUnitId, typeDeclId);
        }
      }

      // Write Template Result (i.e. the "body" of the template, which is called "result" in gcc speak)
      tree templateResultDecl = DECL_TEMPLATE_RESULT(templateDecl);
      int treeCode = TREE_CODE(templateResultDecl);

      if (!DECL_P(templateResultDecl))
      {
        std::string msg(str(boost::format("AST Node (%1%) in %2%") % tree_code_name[treeCode] % BOOST_CURRENT_FUNCTION));
        writeUnsupportedComment(msg);
      }

      switch (treeCode)
      {
        case FUNCTION_DECL:
        {
          // Write CallableUnit
          long callableUnitId =
              writeKdmCallableUnit(templateResultDecl /*tree const functionDecl*/, SkipKdmContainsRelation, true /*bool isTemplate*/);
          writeTripleContains(templateUnitId, callableUnitId);
          break;
        }
        case TYPE_DECL:
        {
          tree typeNode = TREE_TYPE (templateResultDecl);
          long resultId = writeKdmRecordType(typeNode, SkipKdmContainsRelation, true /*bool isTemplate*/);
          writeTripleContains(templateUnitId, resultId);
          break;
        }
        case VAR_DECL:
        case FIELD_DECL:
        case PARM_DECL:
        case CONST_DECL:
        case TEMPLATE_DECL:
        case LABEL_DECL:
        default:
        {
          std::string msg(str(boost::format("AST Declaration Node (%1%) in %2%") % tree_code_name[treeCode] % BOOST_CURRENT_FUNCTION));
          writeUnsupportedComment(msg);
          return;
        }
      }
      markNodeAsProcessed(templateDeclType);
    }
    markNodeAsProcessed(templateParms);
  }
}

long KdmTripleWriter::processAstTypeNode(tree const typeNode, ContainsRelationPolicy const containPolicy, bool isTemplate, const long containedInId)
{
  assert(TYPE_P(typeNode));
  long id = invalidId;
  int treeCode = TREE_CODE(typeNode);
  switch (treeCode)
  {
    case ARRAY_TYPE:
    {
      writeKdmArrayType(typeNode);
      break;
    }
    case FUNCTION_TYPE:
      //Fall through
    case METHOD_TYPE:
    {
      writeKdmSignatureType(typeNode);
      break;
    }
    case REFERENCE_TYPE:
      writeComment("NOTE: This is actually a reference type, not a pointer type does that matter for KDM?");
      //Fall Through
    case POINTER_TYPE:
    {
      id = writeKdmPointerType(typeNode, containPolicy, isTemplate, containedInId);
      break;
    }
    case VECTOR_TYPE:
      //Fall through
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
    case TYPENAME_TYPE:
    case TEMPLATE_TYPE_PARM:
    {
      long compilationUnitId(getSourceFileReferenceId(typeNode));
      id = getReferenceId(typeNode);
      writeTripleKdmType(id, KdmType::TemplateType());

      {
        tree identifierNode = TYPE_IDENTIFIER (typeNode);
        std::string name;
        if (identifierNode)
        {
          name = IDENTIFIER_POINTER(identifierNode);
        }
        else
        {
          name = gcckdm::constants::getUnamedNodeString();
        }
        writeTripleName(id, name);
        writeTripleLinkId(id, name);
      }

      writeKdmSourceRef(id, typeNode);

      if (containPolicy == WriteKdmContainsRelation)
        writeTripleContains(compilationUnitId, id);

      if (containedInId != invalidId)
      {
        assert(containedInId >= 0);
        writeTripleContains(containedInId, id);
      }

      break;
    }

    case UNION_TYPE:
      //Fall Through
    case RECORD_TYPE:
    {
      id = processAstRecordTypeNode(typeNode);
      break;
    }

    case TYPEOF_TYPE:
    {
      //      id = processAstTypeNode(TYPEOF_TYPE_EXPR(typeNode), containPolicy, isTemplate, containedInId);
      break;
    }

    default:
    {
      std::string msg(str(boost::format("AST Type Node (%1%) in %2%") % tree_code_name[treeCode] % BOOST_CURRENT_FUNCTION));
      writeUnsupportedComment(msg);
      break;
    }
  }
  return id;
}


long KdmTripleWriter::processAstRecordTypeNode(tree const typeNode)
{
  long id = invalidId;
  if (isFrontendCxx())
  {
    if (!CLASSTYPE_IS_TEMPLATE(typeNode))
    {
      // This is a struct or class type.
      id = writeKdmRecordType(typeNode);
    }
    else
    {
      // This is a class template.  We don't want to dump it.

      /* We do not output anything (at the moment at least) for templates themselves, only when they are instantiated.
       *   std::string msg(str(boost::format("AST Type Node Class Template (%1%) in %2%") % tree_code_name[treeCode] % BOOST_CURRENT_FUNCTION));
       *   writeUnsupportedComment(msg);
       */
    }
  }
  else
  {
    // This is a struct or class type.
    id = writeKdmRecordType(typeNode);
  }
  return id;
}


void KdmTripleWriter::writeEnumType(tree const enumType)
{
  long enumId = getReferenceId(enumType);
  writeTripleKdmType(enumId, KdmType::EnumeratedType());

  std::string enumName = nodeName(enumType);
  std::string linkName = (enumName == gcckdm::constants::getUnamedNodeString()) ? getLinkIdForType(enumType) : enumName;
  writeTripleName(enumId, enumName);
  writeTripleLinkId(enumId, linkName);

  for (tree tv = TYPE_VALUES (enumType); tv; tv = TREE_CHAIN (tv))
  {
    //
    // The following source code
    // triggers INTEGER_CST handling branch when compiled with cc1,
    // and CONST_DECL handling branch when compiled with cc1plus:
    //
    //   enum E {
    //     a,
    //     b,
    //     c,
    //   };
    //   enum E e;
    //   int main() { return 0; }
    //
    if (TREE_CODE (TREE_VALUE (tv)) == INTEGER_CST)
    {
      long valueId = getNextElementId();
      int value = TREE_INT_CST_LOW (TREE_VALUE (tv));
      writeTripleKdmType(valueId, KdmType::Value());
      std::string name = boost::lexical_cast<std::string>(value);
      writeTripleName(valueId, name);
      writeTripleLinkId(valueId, name);
      writeTriple(valueId, KdmPredicate::Type(), KdmType::IntegerType());
      writeTriple(valueId, KdmPredicate::EnumName(), nodeName(TREE_PURPOSE(tv)));
      writeTripleContains(enumId, valueId);
    }
    else if (TREE_CODE (TREE_VALUE (tv)) == CONST_DECL)
    {
      processAstNodeInternal(tv);
    }
    else
    {
      int treeCode(TREE_CODE(TREE_VALUE (tv)));
      std::string msg(str(boost::format("AST Type Node Enum (%1%) in %2%") % tree_code_name[treeCode] % BOOST_CURRENT_FUNCTION));
      writeUnsupportedComment(msg);
    }
  }
  long compilationUnitId = getSourceFileReferenceId(enumType);
  writeKdmSourceRef(enumId, enumType);
  writeTripleContains(compilationUnitId, enumId);
}

void KdmTripleWriter::processAstVariableDeclarationNode(tree const varDeclaration)
{
  writeKdmStorableUnit(varDeclaration, WriteKdmContainsRelation);
  writeKdmStorableUnitKindGlobal(varDeclaration);
}

void KdmTripleWriter::processAstFunctionDeclarationNode(tree const functionDecl, ContainsRelationPolicy const containPolicy, bool isTemplate)
{
  writeKdmCallableUnit(functionDecl, containPolicy, isTemplate);
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
    if (context)
    {
      // If the item belongs to a class, then the class is the parent
      int treeCode(TREE_CODE(context));
      if (treeCode == RECORD_TYPE)
      {
        writeKdmMemberUnit(fieldDecl);
      }
      else if (treeCode == UNION_TYPE)
      {
        writeKdmItemUnit(fieldDecl);
      }
      else
      {
        std::string msg(str(boost::format("FIXME: %1% : Expected RECORD_TYPE or UNION_TYPE. %2%:%3%") % mCompilationFile % BOOST_CURRENT_FUNCTION % __LINE__));
        writeComment(msg);
      }
    }
  }
  else
  {
    writeKdmItemUnit(fieldDecl);
  }
}

long KdmTripleWriter::processAstValueNode(tree const val, ContainsRelationPolicy const containPolicy)
{
  long valueId = writeKdmValue(val);
  return valueId;
}

/**
 *
 */
void KdmTripleWriter::processAstNamespaceNode(tree const namespaceDecl)
{
  tree decl;
  cp_binding_level* level(NAMESPACE_LEVEL (namespaceDecl));

  // Traverse declarations.
  //
  for (decl = level->names; decl != 0; decl = TREE_CHAIN (decl))
  {
    if (DECL_IS_BUILTIN (decl))
    {
      continue;
    }

    if (!errorcount)
    {
      processAstNodeInternal(decl);
    }
  }

  // Traverse namespaces.
  //
  for (decl = level->namespaces; decl != 0; decl = TREE_CHAIN (decl))
  {
    if (DECL_IS_BUILTIN (decl))
    {
      continue;
    }

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

long KdmTripleWriter::writeKdmCallableUnit(long const callableUnitId, tree functionDecl, ContainsRelationPolicy const containPolicy, bool isTemplate,
    SignatureUnitPolicy signaturePolicy)
{
  std::string name = nodeName(functionDecl);

  if (isFrontendCxx())
  {
    std::string linkSnkStr = linkCallablePrefix + gcckdm::getLinkId(functionDecl, name);

    if (DECL_CONSTRUCTOR_P(functionDecl))
    {
      writeTripleKdmType(callableUnitId, KdmType::MethodUnit());
      writeTripleKind(callableUnitId, KdmKind::Constructor());
      //Identify this as a sink
      if (!isTemplate)
      {
        writeTriple(callableUnitId, KdmPredicate::LinkSnk(), linkSnkStr);
      }
    }
    else if (DECL_DESTRUCTOR_P(functionDecl))
    {
      writeTripleKdmType(callableUnitId, KdmType::MethodUnit());
      writeTripleKind(callableUnitId, KdmKind::Destructor());
      //Identify this as a sink
      if (!isTemplate)
      {
        writeTriple(callableUnitId, KdmPredicate::LinkSnk(), linkSnkStr);
      }
    }
    else if (DECL_OVERLOADED_OPERATOR_P(functionDecl))
    {
      writeTripleKdmType(callableUnitId, KdmType::MethodUnit());
      writeTripleKind(callableUnitId, KdmKind::Operator());
      //Identify this as a sink
      if (!isTemplate)
      {
        writeTriple(callableUnitId, KdmPredicate::LinkSnk(), linkSnkStr);
      }
    }
    else if (DECL_FUNCTION_MEMBER_P(functionDecl))
    {
      writeTripleKdmType(callableUnitId, KdmType::MethodUnit());
      writeTripleKind(callableUnitId, KdmKind::Method());
      //Identify this as a sink
      if (!isTemplate)
      {
        writeTriple(callableUnitId, KdmPredicate::LinkSnk(), linkSnkStr);
      }
    }
    else
    {
      writeTripleKdmType(callableUnitId, KdmType::CallableUnit());
      if (DECL_REALLY_EXTERN(functionDecl))
      {
        writeTripleKind(callableUnitId, KdmKind::External());
      }
      else
      {
        writeTripleKind(callableUnitId, KdmKind::Regular());
        //Identify this as a sink
        if (!isTemplate)
        {
          writeTriple(callableUnitId, KdmPredicate::LinkSnk(), linkSnkStr);
        }
      }
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
    if (!isTemplate)
    {
      writeTripleLinkId(callableUnitId, gcckdm::getLinkId(functionDecl, name));
    }
  }
  else
  {
    writeTripleKdmType(callableUnitId, KdmType::CallableUnit());
    if (DECL_EXTERNAL(functionDecl))
    {
      writeTripleKind(callableUnitId, KdmKind::External());
    }
    else
    {
      writeTripleKind(callableUnitId, KdmKind::Regular());
      if (!isTemplate)
      {
        //Identify this as a sink
        writeTriple(callableUnitId, KdmPredicate::LinkSnk(), linkCallablePrefix + gcckdm::getLinkId(functionDecl, name));
      }
    }
    if (!isTemplate)
    {
      // Standard C does not require mangled names for link:id
      writeTripleLinkId(callableUnitId, name);
    }
  }

  writeTripleName(callableUnitId, name);

  if (DECL_DECLARED_INLINE_P (functionDecl))
  {
    // Inline keyword
    writeTriple(callableUnitId, KdmPredicate::Stereotype(), KdmElementId_InlineStereotype);
  }

  // Const keyword
  writeKdmTypeQualifiers(functionDecl);

  if (!DECL_NONSTATIC_MEMBER_FUNCTION_P (functionDecl))
  {
    // Static keyword
    writeTriple(callableUnitId, KdmPredicate::Stereotype(), KdmElementId_StaticStereotype);
  }

  // Source reference
  writeKdmSourceRef(callableUnitId, functionDecl);

  // If this is c++, the method/function is written in the class first, otherwise in the compilation unit
  if (isFrontendCxx())
  {
    tree context = CP_DECL_CONTEXT (functionDecl);
    // If the context is a type, then get the visibility
    if (context)
    {
      if (TYPE_P(context))
      {
        if (TREE_PRIVATE (functionDecl))
        {
          writeTripleExport(callableUnitId, "private");
        }
        else if (TREE_PROTECTED (functionDecl))
        {
          writeTripleExport(callableUnitId, "protected");
        }
        else
        {
          writeTripleExport(callableUnitId, "public");
        }
      }
    }

    // Explicit keyword
    if (DECL_NONCONVERTING_P (functionDecl))
      writeTriple(callableUnitId, KdmPredicate::Stereotype(), KdmElementId_ExplicitStereotype);

    // Explicit keyword
    if (DECL_ARTIFICIAL (functionDecl))
      writeTriple(callableUnitId, KdmPredicate::Stereotype(), KdmElementId_HiddenStereotype);

    // Friendship
#if 1 //BBBBB
    writeKdmFriends(callableUnitId, functionDecl);
#else
    writeKdmFriends(callableUnitId, DECL_BEFRIENDING_CLASSES(functionDecl));
#endif

    // Containment
    if (containPolicy == WriteKdmContainsRelation)
      writeKdmCxxContains(callableUnitId, functionDecl);
  }
  else
  {
    // Containment
    if (containPolicy == WriteKdmContainsRelation)
    {
      // In straight C, it is always contained in the source file
      long unitId = getSourceFileReferenceId(functionDecl);
      writeTripleContains(unitId, callableUnitId);
    }
  }

  lockUid(true);

  /* When a file has NO functions in it, gcc seems to consider the file completely empty
   * and we can NOT get to extract anything at all from that file.
   * To work around this, we have our pre-processing phase add a dummy/empty function
   * with a fixed name. Then we mark that function hidden here. */
  if (name == "KDM_7222aa80eacbe6ed1bab6665910fb059")
    writeTriple(callableUnitId, KdmPredicate::Stereotype(), KdmElementId_HiddenStereotype);

  if (signaturePolicy == WriteSignatureUnit)
  {
    long signatureId = writeKdmSignature(functionDecl);
    writeTripleContains(callableUnitId, signatureId);
    writeTriple(callableUnitId, KdmPredicate::Type(), signatureId);
  }

  //If we are skipping the signature
  if (!isTemplate && signaturePolicy != SkipSignatureUnit)
  {
    if (mSettings.functionBodies)
    {
      mGimpleWriter->processAstFunctionDeclarationNode(functionDecl);
    }
  }

  lockUid(false);

  return callableUnitId;
}


/**
 * Just get the reference of the functionDecl and pass it along
 */
long KdmTripleWriter::writeKdmCallableUnit(tree const functionDecl, ContainsRelationPolicy const containPolicy, bool isTemplate)
{
  return writeKdmCallableUnit(getReferenceId(functionDecl), functionDecl, containPolicy, isTemplate, WriteSignatureUnit);
}


void KdmTripleWriter::lockUid(bool val)
{
  mLockUid = val;
}


bool KdmTripleWriter::lockUid() const
{
  return mLockUid;
}


/**
 * C++ containment is more complicated then standard C containment. In KDM we follow
 * the following procedure:
 *
 * If the context is a class and decl is a function/method
 *    if decl is in the same file as the context class the parent is the class
 *    otherwise put a fake callable in the class and the parent is the compilation/shared unit
 */
void KdmTripleWriter::writeKdmCxxContains(long declId, tree const decl)
{
  if (isFrontendCxx())
  {
    tree context = CP_DECL_CONTEXT (decl);
    // If the context is a type, then get the visibility
    if (context)
    {
      // If the item belongs to a class, then the class is the parent
      int treeCode(TREE_CODE(context));
      if (treeCode == RECORD_TYPE)
      {
        long classUnitId = getReferenceId(context);
        if (TREE_CODE(decl) == FUNCTION_DECL)
        {
          //this decl is in the same file as the context... to make a prettier diagram
          //we stuff this decl in the class
          if (getSourceFileReferenceId(context) == getSourceFileReferenceId(decl))
          {
            writeTripleContains(classUnitId, declId);
            return;
          }
          //We are not in the same file as our context, therefore we stick a fake callable in
          //our context and the _real_ parent is the the compilation/shared unit where
          //the decl is actually defined.
          else
          {
            long fakeId = getNextElementId();
            //We skip writing a signature for this fake callable since at the moment it leads to
            //double containment problems.
            writeKdmCallableUnit(fakeId, decl, SkipKdmContainsRelation, false, SkipSignatureUnit);
            writeTripleContains(classUnitId, fakeId);
          }
        }
      }
    }
  }

  // Otherwise the file is the parent
  Path sourceFile(DECL_SOURCE_FILE(decl));
  long unitId(sourceFile == mCompilationFile ? KdmElementId_CompilationUnit : getSourceFileReferenceId(decl));
  writeTripleContains(unitId, declId);
}

long KdmTripleWriter::writeKdmSignatureDeclaration(tree const functionDecl)
{
  std::string name(nodeName(functionDecl));
  long signatureId = getNextElementId();
  writeTripleKdmType(signatureId, KdmType::Signature());
  writeTripleName(signatureId, name);
  //Determine return type id
  //  tree t(TREE_TYPE (TREE_TYPE (functionDecl)));
  //  tree t2 = typedefTypeCheck(TREE_TYPE (functionDecl));
  //(TYPE_MAIN_VARIANT(t));
  long paramId = writeKdmReturnParameterUnit(TREE_TYPE (functionDecl));
  writeTripleContains(signatureId, paramId);

  //Iterator through argument list
  tree arg = DECL_ARGUMENTS (functionDecl);
  tree argType = TYPE_ARG_TYPES (TREE_TYPE (functionDecl));
  //While it would be reasonable to not allow an argument without a type
  //C apparently allows this sort of code:
  //
  //int foo(depth)
  //{
  //  return depth +1;
  //}
  // Therefore we have to check to ensure that we don't have
  // arg or type before exiting the loop
  int count = 0;
  while ((arg || argType) && (argType != void_list_node))
  {
    if (arg)
    {
      long refId = writeKdmParameterUnit(arg);
      writeKdmTypeQualifiers(arg);
      writeTriplePosition(refId, count++);
      writeTripleContains(signatureId, refId);
      arg = TREE_CHAIN (arg);
    }
    if (argType)
    {
      //      long ref = getReferenceId(TREE_VALUE(argType));
      //      writeRelation(KdmType::HasType(), signatureId, ref);
      argType = TREE_CHAIN (argType);
    }
  }

  // Throws
  if (gcckdm::isFrontendCxx())
  {
    tree ft = TREE_TYPE (functionDecl);
    tree raises = TYPE_RAISES_EXCEPTIONS (ft);
    if (raises)
    {
      if (TREE_VALUE (raises))
      {
        for (; raises != NULL_TREE; raises = TREE_CHAIN (raises))
        {
          long raisesId = getReferenceId(TREE_VALUE (raises));
          long throwId = writeKdmThrows(raisesId);
          writeTripleContains(signatureId, throwId);
        }
      }
    }
  }
  markNodeAsProcessed(functionDecl);
  return signatureId;
}

/**
 *
 */
void KdmTripleWriter::writeTriplePosition(long const id, int pos)
{
  writeTriple(id, KdmPredicate::Pos(), boost::lexical_cast<std::string>(pos));
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
  writeTripleLinkId(signatureId, name);

  //Determine return type
  long paramId = writeKdmReturnParameterUnit(TREE_TYPE(functionType));
  writeTripleContains(signatureId, paramId);

  //Iterator through argument list
  tree argType = TYPE_ARG_TYPES (functionType);
  int count = 0;
  while (argType && (argType != void_list_node))
  {
    long refId = writeKdmParameterUnit(argType, true);
    writeTriplePosition(refId, count++);
    writeTripleContains(signatureId, refId);

    //To ensure that
    //    tree type = NULL_TREE;
    //    if (TREE_TYPE(argType))
    //    {
    //      type = typedefTypeCheck(param);
    //    }
    //    else
    //    {
    //      type = TYPE_MAIN_VARIANT(TREE_VALUE (param));
    //    }
    //    long ref = getReferenceId(TREE_VALUE(argType));
    //    writeRelation(KdmType::HasType(), signatureId, ref);

    argType = TREE_CHAIN (argType);
  }

  tree context = TYPE_CONTEXT(functionType);
  if (context)
  {
    writeTripleContains(getReferenceId(TYPE_CONTEXT(functionType)), signatureId);
  }
  else
  {
    writeTripleContains(KdmElementId_CompilationUnit, signatureId);
  }

  writeTriple(signatureId, KdmPredicate::Stereotype(), KdmElementId_HiddenStereotype);

  markNodeAsProcessed(functionType);

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
  writeComment("KDM File created with GccKdmPlugin Version: " + gcckdm::GccKdmVersion);
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

#if 1 //BBBB
  writeTriple(KdmElementId_Audit, KdmPredicate::KdmType(), KdmType::Audit());
  {
    std::string linkIdStr = (KdmType::Audit()).name() + ":" + gcckdm::GccKdmVersion;
    writeTriple(KdmElementId_Audit, KdmPredicate::Name(), linkIdStr /*KdmType::Audit()*/);
    writeTriple(KdmElementId_Audit, KdmPredicate::Version(), gcckdm::GccKdmVersion);
    writeTriple(KdmElementId_Audit, KdmPredicate::LinkId(), linkIdStr);
  }
  writeTripleContains(KdmElementId_Segment, KdmElementId_Audit);
#endif

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
  writeTriple(KdmElementId_CxxExtensionFamily, KdmPredicate::KdmType(), KdmType::ExtensionFamily());
  writeTriple(KdmElementId_CxxExtensionFamily, KdmPredicate::Name(), "C/C++");
  writeTriple(KdmElementId_CxxExtensionFamily, KdmPredicate::LinkId(), "C/C++");
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

  writeTriple(KdmElementId_BuiltinStereotype, KdmPredicate::KdmType(), KdmType::Stereotype());
  writeTriple(KdmElementId_BuiltinStereotype, KdmPredicate::Name(), "builtin");
  writeTriple(KdmElementId_BuiltinStereotype, KdmPredicate::LinkId(), "builtin");
  writeTripleContains(KdmElementId_CxxExtensionFamily, KdmElementId_BuiltinStereotype);

  // Code Model contents
  writeTriple(KdmElementId_CodeAssembly, KdmPredicate::KdmType(), KdmType::CodeAssembly());
  writeTriple(KdmElementId_CodeAssembly, KdmPredicate::Name(), ":code");
  writeTriple(KdmElementId_CodeAssembly, KdmPredicate::LinkId(), ":code");
  writeTripleContains(KdmElementId_CodeModel, KdmElementId_CodeAssembly);

  writeTriple(KdmElementId_LanguageUnit, KdmPredicate::KdmType(), KdmType::LanguageUnit());
  writeTriple(KdmElementId_LanguageUnit, KdmPredicate::Name(), ":language");
  writeTriple(KdmElementId_LanguageUnit, KdmPredicate::LinkId(), ":language");
  writeTripleContains(KdmElementId_CodeAssembly, KdmElementId_LanguageUnit);

  writeTriple(KdmElementId_InventoryModel, KdmPredicate::KdmType(), KdmType::InventoryModel());
  writeTriple(KdmElementId_InventoryModel, KdmPredicate::Name(), KdmType::InventoryModel());
  writeTriple(KdmElementId_InventoryModel, KdmPredicate::LinkId(), KdmType::InventoryModel());
  writeTripleContains(KdmElementId_Segment, KdmElementId_InventoryModel);

  writeKdmCompilationUnit(mCompilationFile);
  //include the compilation file in the inventory model
  writeKdmSourceFile(mCompilationFile);
}

void KdmTripleWriter::writeTripleKdmType(long const subject, KdmType const & object)
{
  writeTriple(subject, KdmPredicate::KdmType(), object);
}

void KdmTripleWriter::writeTripleName(long const subject, std::string const & name)
{
  writeTriple(subject, KdmPredicate::Name(), name);
}

void KdmTripleWriter::writeKdmBuiltinStereotype(long const subject)
{
  writeTriple(subject, KdmPredicate::Stereotype(), KdmElementId_BuiltinStereotype);
}

void KdmTripleWriter::writeTripleContains(long const parent, long const child, bool uid)
{
  bool result;
  if (uid)
  {
    //add the node(s) to the uid graph
    result = updateUidGraph(parent, child);
  }
  else
  {
    //User requested no UID generation
    result = true;
  }

  if (result)
  {
    //write contains relationship to sink
    writeTriple(parent, KdmPredicate::Contains(), child);
  }
}

/**
 *
 */
long KdmTripleWriter::writeKdmExtends(long const subclass, long const superclass)
{
  long extendsId = getNextElementId();
  writeTripleKdmType(extendsId, KdmType::Extends());
  writeTriple(extendsId, KdmPredicate::From(), subclass);
  writeTriple(extendsId, KdmPredicate::To(), superclass);
  return extendsId;
}

/**
 *
 */
long KdmTripleWriter::writeKdmFriend(long const subclass, long const superclass)
{
  long extendsId = getNextElementId();
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
    mUidGraph[parentVertex].incrementFlag = !lockUid();
  }

  if (not foundChildFlag)
  {
    //Create our child vertex
    childVertex = boost::add_vertex(mUidGraph);
    mUidGraph[childVertex].elementId = child;
    mUidGraph[childVertex].incrementFlag = !lockUid();
  }

  bool retVal = true;
  if (mSettings.containmentCheck)
  {
    ContainmentMap::const_iterator ci = mContainment.find(child);
    if (ci != mContainment.end())
    {
      std::string msg(str(boost::format("FIXME: %1% : Double containment element <%2%> is contained in <%3%> and <%4%>. %5%:%6%") % mCompilationFile % child % ci->second % parent % BOOST_CURRENT_FUNCTION % __LINE__));
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
  Path sourceFile = checkPathType(mSettings, file);
  long id = getNextElementId();
  writeTripleKdmType(id, KdmType::SourceFile());
  writeTripleName(id, sourceFile.filename());
  writeTriple(id, KdmPredicate::Path(), sourceFile.string());
  writeTripleLinkId(id, sourceFile.string());

  writeTripleContains(getDirectoryId(sourceFile.parent_path()), id, false);

  //Keep track of all files inserted into the inventory model
  std::pair<FileMap::iterator, bool> result = mInventoryMap.insert(std::make_pair(sourceFile, id));

  return result.first;
}

void KdmTripleWriter::writeKdmCompilationUnit(Path const & file)
{
  Path tmp = checkPathType(mSettings, file);
  writeTripleKdmType(KdmElementId_CompilationUnit, KdmType::CompilationUnit());
  writeTripleName(KdmElementId_CompilationUnit, tmp.filename());
  writeTripleLinkId(KdmElementId_CompilationUnit, tmp.string());
  writeTripleContains(getPackageId(tmp.parent_path()), KdmElementId_CompilationUnit);
}

long KdmTripleWriter::getDirectoryId(Path const & directoryDir)
{
  return getLocationContextId(directoryDir, KdmElementId_InventoryModel, mDirectoryMap, KdmType::Directory());
}

long KdmTripleWriter::getPackageId(Path const & packageDir)
{
  return getLocationContextId(packageDir, KdmElementId_CodeAssembly, mPackageMap, KdmType::Package());
}

long KdmTripleWriter::getLocationContextId(Path const & contextDir, long const rootId, FileMap & fMap, KdmType const & type)
{
  long parentContextId = invalidId;
  long contextId = invalidId;
  Path normalizedContextDir = contextDir;
  normalizedContextDir.normalize();
  FileMap::iterator i = fMap.find(normalizedContextDir);
  //If we haven't encountered this path before...
  if (i == fMap.end())
  {
    //if we don't have a path to iterate through... the context is the root
    if (!normalizedContextDir.empty())
    {
      // Iterate through the path from left to right building the path to the contextDir on piece at a
      // time.  Each piece is checked to see if it has already been encountered if it hasn't it is
      // written out to file and added to the appropriate parent then added to the cached paths.
      Path builtPath;
      for (Path::iterator pIter = normalizedContextDir.begin(); pIter != normalizedContextDir.end(); ++pIter)
      {
        builtPath = builtPath / *pIter;
        std::pair<FileMap::iterator, bool> result = fMap.insert(std::make_pair(builtPath, mKdmElementId + 1));
        if (result.second)
        {
          contextId = getNextElementId();
          writeTripleKdmType(contextId, type);
          writeTripleLinkId(contextId, *pIter);
          writeTripleName(contextId, *pIter);
          //We have encountered a completely new path....add to the root
          if (parentContextId == invalidId)
          {
            writeTripleContains(rootId, contextId);
          }
          //Add to the found parent
          else
          {
            writeTripleContains(parentContextId, contextId);
          }
          parentContextId = contextId;
          fMap.insert(std::make_pair(builtPath, contextId));
        }
        else
        {
          parentContextId = result.first->second;
        }
      }
    }
    //We don't have a root path
    else
    {
      contextId = rootId;
    }
  }
  else
  {
    contextId = i->second;
  }
  return contextId;
}


long KdmTripleWriter::writeKdmReturnParameterUnit(tree const param)
{
  tree type = TREE_TYPE(param);
  if (!type) {
    type = TYPE_MAIN_VARIANT(param);
  } else {
    type = getTypeNode(type);
  }
  long ref = getReferenceId(type);
  long subjectId = getNextElementId();
  writeTripleKdmType(subjectId, KdmType::ParameterUnit());
  writeTripleName(subjectId, "__RESULT__");
  writeTripleKind(subjectId, KdmKind::Return());
  writeTriple(subjectId, KdmPredicate::Type(), ref);
  // For the moment ensure that all parameters have the hasType relationship
  // this may result is a huge explosion of data that we eventually might
  // to restrict to just non-primitive types
  writeRelation(KdmType::HasType(), subjectId, ref);

  return subjectId;
}


long KdmTripleWriter::writeKdmParameterUnit(tree const param, bool forceNewElementId)
{
  if (!param)
  {
    BOOST_THROW_EXCEPTION(NullAstNodeException());
  }

  long parameterUnitId;

  // GCC reuses param nodes when outputting function types.. this causes
  // many double containments... so when outputting parameter units for types
  // do not look up the refId we create a new one for each parameter which
  // is safe because they _shouldn't_ be referenced anywhere.
  parameterUnitId = (forceNewElementId) ? getNextElementId() : getReferenceId(param);

  writeTripleKdmType(parameterUnitId, KdmType::ParameterUnit());

  tree type = TREE_TYPE(param);
  if (!type) {
    type = TYPE_MAIN_VARIANT(TREE_VALUE (param));
  } else {
    type = getTypeNode(type);
  }

  long ref = getReferenceId(type);

  std::string name(nodeName(param));
  writeTripleName(parameterUnitId, name);

  writeTriple(parameterUnitId, KdmPredicate::Type(), ref);
  // For the moment ensure that all parameters have the hasType relationship
  // this may result is a huge explosion of data that we eventually might
  // to restrict to just non-primitive types
  writeRelation(KdmType::HasType(), parameterUnitId, ref);
  return parameterUnitId;
}


/**
 * Write the memberUnit and any associated data.
 */
long KdmTripleWriter::writeKdmMemberUnit(tree const member)
{
  long memberId = getReferenceId(member);
  writeTripleKdmType(memberId, KdmType::MemberUnit());
  tree type = TREE_TYPE(member);
  if (!type) {
//    std::string msg(str(boost::format("Member node <%1%>: TREE_TYPE()==NULL in %2%:%3%") % getReferenceId(member) % BOOST_CURRENT_FUNCTION % __LINE__));
//    writeUnsupportedComment(msg);
    type = TYPE_MAIN_VARIANT(member);
  } else {
    type = getTypeNode(type);
  }
  long ref = getReferenceId(type);
  std::string name(nodeName(member));

  writeTripleName(memberId, name);
  writeTripleLinkId(memberId, name);
  writeTriple(memberId, KdmPredicate::Type(), ref);
  writeKdmSourceRef(memberId, member);

  // Set the export kind, if available
  tree context = CP_DECL_CONTEXT (member);
  if (TYPE_P(context))
  {
    if (TREE_PRIVATE (member))
    {
      writeTripleExport(memberId, "private");
    }
    else if (TREE_PROTECTED (member))
    {
      writeTripleExport(memberId, "protected");
    }
    else
    {
      writeTripleExport(memberId, "public");
    }
  }

  writeTripleContains(getReferenceId(context), memberId);

  return memberId;
}

long KdmTripleWriter::writeKdmItemUnit(tree const item)
{
  long itemId = getReferenceId(item);
  writeTripleKdmType(itemId, KdmType::ItemUnit());

  tree type = TREE_TYPE(item);
  if (!type) {
    type = TYPE_MAIN_VARIANT(item);
  } else {
    type = getTypeNode(type);
  }
  long ref = getReferenceId(type);
  std::string name(nodeName(item));

  writeTripleName(itemId, name);
  writeTripleLinkId(itemId, name);
  writeTriple(itemId, KdmPredicate::Type(), ref);
  writeKdmSourceRef(itemId, item);
  tree context = DECL_CONTEXT(item);
  writeTripleContains(getReferenceId(context), itemId);
  return itemId;
}

void KdmTripleWriter::writeKdmStorableUnitKindGlobal(tree const var)
{
  long unitId = getReferenceId(var);
#if 0 //BBBB - TMP
  if (unitId == 115) {
	int junk = 123;
  }
#endif
  std::string name = nodeName(var);

  if (DECL_EXTERNAL(var)) {
    writeTripleKind(unitId, KdmKind::External());
  } else {
    writeTripleKind(unitId, KdmKind::Global());
    if (TREE_PUBLIC (var)) {
      std::string linkSnkStr = linkVariablePrefix + gcckdm::getLinkId(var, name);
      writeTriple(unitId, KdmPredicate::LinkSnk(), linkSnkStr);
    }
  }

  writeKdmStorableUnitInitialization(var);
}

void KdmTripleWriter::writeKdmStorableUnitInitialization(tree const var)
{
  if (!DECL_EXTERNAL(var)) {
    //We now check to see if this variable is initialized
    //  example int e[] = { 1, 2, 3 }
    //  MyFunctionPtrType  f[] = { foo, bar }
    tree declInitial = DECL_INITIAL(var);

    if (declInitial) {
      long blockId = getNextElementId();
      writeTripleKdmType(blockId, KdmType::BlockUnit());
      writeTripleKind(blockId, KdmKind::Init());
      writeKdmSourceRef(blockId, var);

      long unitId = getReferenceId(var);
      writeTripleContains(unitId, blockId);

      lockUid(true);

      if (TREE_CODE(declInitial) == CONSTRUCTOR) {
          GimpleKdmTripleWriter::ActionDataPtr declInitialActionDataPtr =
        		mGimpleWriter->writeKdmUnaryConstructor(NULL_TREE /*lhs*/, declInitial /*rhs*/, gcckdm::locationOf(declInitial), var /*lhs_var*/, NULL_TREE /*lhs_var_DE*/, blockId /*containingId*/);
      } else {
    	  declInitial = mGimpleWriter->skipAllNOPsEtc(declInitial);
          GimpleKdmTripleWriter::ActionDataPtr declInitialActionDataPtr =
          		mGimpleWriter->writeKdmUnaryConstructor(var /*lhs*/, declInitial /*rhs*/, gcckdm::locationOf(declInitial), var /*lhs_var*/, NULL_TREE /*lhs_var_DE*/, blockId /*containingId*/);
      }

      lockUid(false);
    }
  }
}

long KdmTripleWriter::writeKdmStorableUnit(tree const var, ContainsRelationPolicy const containPolicy)
{
  long unitId = getReferenceId(var);
#if 0 //BBBB - TMP
  if (unitId == 68675) {
    int junk = 123;
  }
#endif
  writeTripleKdmType(unitId, KdmType::StorableUnit());

  std::string name = nodeName(var);
  writeTripleName(unitId, name);

  tree type0 = TREE_TYPE(var);
  tree type = getTypeNode(type0);
  long ref = getReferenceId(type);

  writeTriple(unitId, KdmPredicate::Type(), ref);

  //For the moment we only want structures to have hasType relationship
  if (TREE_CODE(TYPE_MAIN_VARIANT(TREE_TYPE(var))) == RECORD_TYPE) {
    writeRelation(KdmType::HasType(), unitId, ref);
  }

  if (DECL_ARTIFICIAL (var /*functionDecl*/)) {
    writeTriple(unitId, KdmPredicate::Stereotype(), KdmElementId_HiddenStereotype);
  }

  writeKdmSourceRef(unitId, var);

  if (containPolicy == WriteKdmContainsRelation) {
    writeTripleContains(getSourceFileReferenceId(var), unitId);
  }

  if (TREE_PUBLIC (var)) {
    std::string linkIdStr = gcckdm::getLinkId(var, name);
    writeTripleLinkId(unitId, linkIdStr);
  }

  return unitId;
}

long KdmTripleWriter::writeKdmValue(tree const val)
{
  long valueId = getNextElementId();
  tree type0 = TREE_TYPE(val);
  tree type = getTypeNode(type0);
  long ref = getReferenceId(type);
  writeTripleKdmType(valueId, KdmType::Value());
  std::string name = nodeName(val);
  writeTripleName(valueId, name);
  writeTripleLinkId(valueId, name);
  writeTriple(valueId, KdmPredicate::Type(), ref);
  return valueId;
}

bool KdmTripleWriter::hasReferenceId(tree const node) const
{
  TreeMap::const_iterator i = mReferencedNodes.find(node);
  return !(i == mReferencedNodes.end());
}

long KdmTripleWriter::getId(tree const node)
{
	return getReferenceId(node);
}

long KdmTripleWriter::getId_orInvalidIdForNULL(tree const node)
{
	return node ? getReferenceId(node) : invalidId;
}

long KdmTripleWriter::getNextElementId()
{
#if 0 //BBBB - TMP
  if (mKdmElementId + 1 == 68675) {
	int junk = 123;
  }
#endif
  return ++mKdmElementId;
}

long KdmTripleWriter::getReferenceId(tree const node)
{
#if 0 //BBBB - TMP
	if ((long unsigned int)node == 0x7ffff3f6b1f8) {
		int junk = 123;
	}
#endif
  long retValue(-1);
  std::pair<TreeMap::iterator, bool> result = mReferencedNodes.insert(std::make_pair(node, mKdmElementId + 1));
  if (result.second)
  {
    //if we haven't processed this node before new node....queue it node for later processing
    if (mProcessedNodes.find(node) == mProcessedNodes.end())
    {
#if 0 //BBBB - TMP
      if (mKdmElementId + 1 == 115) {
		int junk = 123;
	  }
#endif
      //Keeping this code here for debugging
      //      fprintf(stderr,"mNodeQueue.push: %p <%ld>\n", node, mKdmElementId + 1);
      mNodeQueue.push(node);
    }
    retValue = ++mKdmElementId;
  }
  else
  {
    retValue = result.first->second;
#if 0 //BBBB - TMP
    if (retValue == 76) {
      int junk = 123;
	}
#endif
  }
  return retValue;
}

/**
 * Returns the id of the value, if it's cached...
 * for values that haven't been cached they are processed immediately
 */
long KdmTripleWriter::getValueId(tree const node)
{
  long valueId = processAstValueNode(node, SkipKdmContainsRelation);
  return valueId;
}

long KdmTripleWriter::getSourceFileReferenceId(tree const node)
{
  //The shared or compliation unit id
  long unitId(0);

  //Find the file the given node is located in ensure that is is complete
  expanded_location loc(expand_location(locationOf(node)));
  Path file(loc.file);

  // the node is not in our translation unit, it must be in a shared unit
  if (mCompilationFile != file)
  {
    //Retrieve the identifier node for the file containing the given node
    tree identifierNode = get_identifier(loc.file);
    unitId = getSharedUnitReferenceId(identifierNode);
  }
  else
  {
    //    the node is located in the translation unit; return the id for the translation unit
    //    unless it's external in which case we don't know where it is
    //    in which case we put it in the derivedSharedUnit by default
    //    if (DECL_P(node))
    //    {
    //      if (!DECL_EXTERNAL(node))
    //      {
    //        unitId = KdmElementId_CompilationUnit;
    //      }
    //      else
    //      {
    //        unitId = KdmElementId_DerivedSharedUnit;
    //      }
    //    }
    //    else
    //    {
    //      unitId = KdmElementId_CompilationUnit;
    //    }

#if 0 //BBBBB  This also produces warnings on inline functions defined within class, which is undesirable. To remove completely.
    if (DECL_P(node) && DECL_EXTERNAL(node))
    {
      writeComment("WARNING: External element '" + nodeName(node) + "' found within CompilationUnit '" + mCompilationFile.string());
    }
#endif
    unitId = KdmElementId_CompilationUnit;
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
    mUserTypes.insert(std::make_pair(type, typeId));

    writeTripleKdmType(typeId, type);
    writeTripleName(typeId, type.name());
    writeTripleLinkId(typeId, type.name());

    writeLanguageUnitContains(typeId);
    retVal = typeId;
  }
  else
  {
    retVal = i->second;
  }
  return retVal;
}

bool KdmTripleWriter::nodeIsMarkedAsProcessed(tree const ast)
{
#if 0 //BBBB TMP
  if ((const unsigned long)ast == 0xb7c17e38)
  {
  	int junk = 123;
  }
#endif
  return (mProcessedNodes.find(ast) != mProcessedNodes.end());
}

void KdmTripleWriter::markNodeAsProcessed(tree const ast)
{
#if 0 //BBBB TMP
  if ((const unsigned long)ast == 0xb7c17e38)
  {
  	int junk = 123;
  }
#endif
  mProcessedNodes.insert(ast);
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

void KdmTripleWriter::writeKdmPrimitiveType(tree const type0)
{
  int treeCode = TREE_CODE(type0);
  long typeKdmElementId = getReferenceId(type0);

  tree type;
  std::string name;
  if (treeCode == VECTOR_TYPE) {
    tree type1 = TREE_TYPE(type0);
    type = TYPE_MAIN_VARIANT(type1);
    unsigned int nunits = TYPE_VECTOR_SUBPARTS (type0);
    std::string nunits_str = boost::str(boost::format(HOST_WIDE_INT_PRINT_DEC) % nunits);
    name = nodeName(type) + " : vector_size(" + nunits_str + ")";
  } else {
    type = type0;
    name = nodeName(type);
  }

  KdmType kdmType = KdmType::PrimitiveType();
  if (name.find("int") != std::string::npos ||
      name.find("long") != std::string::npos ||
      name.find("unnamed-unsigned") != std::string::npos ||
      name.find("unnamed-signed") != std::string::npos)
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
  else if (name.find("bool") != std::string::npos || name.find("_Bool") != std::string::npos)
  {
    kdmType = KdmType::BooleanType();
  }
  //  else
  //  {
  //    writeComment(name);
  //  }

  writeTripleKdmType(typeKdmElementId, kdmType);
  writeTripleName(typeKdmElementId, name);
  writeTripleLinkId(typeKdmElementId, name);
  writeLanguageUnitContains(typeKdmElementId);
}

tree KdmTripleWriter::getTypeNode(tree type)
{
  tree type2 = type;
  tree const typeMainVariant = TYPE_MAIN_VARIANT(type);
  if (typeMainVariant != type) {
    tree typeName = TYPE_NAME(type);
    if (typeName) {
      const expanded_location xloc = expand_location(locationOf(typeName));
      if (xloc.file && strcmp("<built-in>", xloc.file)) {
        type2 = typeName;

        tree typeDecl = typeName;
        tree typeNode = TREE_TYPE (typeDecl);
        if (typeNode) {
          int treeCode = TREE_CODE(typeDecl);
          int typeCode = TREE_CODE(typeNode);
          if (treeCode == TYPE_DECL && typeCode == RECORD_TYPE) {
            if (DECL_ARTIFICIAL(typeDecl)) {
              type2 = typeMainVariant;
            }
          }
        }

      } else {
        type2 = typeMainVariant;
      }
    } else {
      type2 = typeMainVariant;
    }
  }
  return type2;
}

long KdmTripleWriter::writeKdmPointerType(tree const pointerType, ContainsRelationPolicy const containPolicy, bool isTemplate,
    const long containedInId)
{
  long pointerKdmElementId = getReferenceId(pointerType);

  writeTripleKdmType(pointerKdmElementId, KdmType::PointerType());

  tree type = TREE_TYPE(pointerType);
  tree t2 = getTypeNode(type);
  long pointerTypeKdmElementId = getReferenceId(t2);

  writeTriple(pointerKdmElementId, KdmPredicate::Type(), pointerTypeKdmElementId);

//  if (!isTemplate) {
//    std::string name = nodeName(t2);
////    std::string linkId = "ptr:" + name;
//    writeTripleLinkId(pointerKdmElementId, name.empty() ? getLinkIdForType(pointerType) : ("ptr: " + name));
////    writeTripleLinkId(pointerKdmElementId, getLinkIdForType(pointerType));
//  }

  if (containPolicy == WriteKdmContainsRelation) {
    writeLanguageUnitContains(pointerKdmElementId);
  }

  if (containedInId != invalidId) {
    assert(containedInId >= 0);
    writeTripleContains(containedInId, pointerKdmElementId);
  }

  return pointerKdmElementId;
}

/*
 * Check the item unit map for the given id, and return the array types item unit id,  
 * if it's not there create an reference id for the item unit, add it to the item unit map
 * and return newly created ItemUnit id
 */
long KdmTripleWriter::getItemUnitId(long arrayTypeId)
{
#if 0 //BBBB - TMP
  if (arrayTypeId == 11554) {
    int junk = 123;
  }
#endif

  ContainmentMap::const_iterator e = mItemUnits.find(arrayTypeId);
  if (e != mItemUnits.end()) {
    return e->second;
  } else {
    return mItemUnits.insert(std::make_pair(arrayTypeId, getNextElementId())).first->second;
  }
}


void KdmTripleWriter::writeKdmArrayType(tree const arrayType)
{
  long arrayKdmElementId = getReferenceId(arrayType);
#if 0 //BBBB - TMP
  if (arrayKdmElementId == 11554) {
    int junk = 123;
  }
#endif
  writeTripleKdmType(arrayKdmElementId, KdmType::ArrayType());

  //create item unit that acts as a placeholder for elements written to or read from this array
  //use the getItemUnitId method to ensure we use the proper id for relationships
  long itemUnitElementId = getItemUnitId(arrayKdmElementId);
  writeTripleKdmType(itemUnitElementId, KdmType::ItemUnit());
  //contain the item unit within the arrayType...hope the importer is smart enough to 
  //use setItemUnit 
  writeTripleContains(arrayKdmElementId, itemUnitElementId);
  
  tree type = TREE_TYPE(arrayType);
  tree t2 = getTypeNode(type);
  long arrayTypeKdmElementId = getReferenceId(t2);
  //writeTriple(arrayKdmElementId, KdmPredicate::Type(), arrayTypeKdmElementId);

  //The type is set on the ItemUnit not the ArrayType itself
  writeTriple(itemUnitElementId, KdmPredicate::Type(), arrayTypeKdmElementId);


//  std::string name = nodeName(t2);
////    std::string linkId = "array:" + name;

////writeTripleLinkId(arrayKdmElementId, name.empty() ? getLinkIdForType(arrayType) : ("array: " + name));

//writeTripleLinkId(arrayKdmElementId, getLinkIdForType(arrayType));
//    writeTripleLinkId(arrayKdmElementId, getLinkIdForType(type));
//    writeTripleLinkId(arrayKdmElementId, getLinkIdForType(t2));
//        tree typeName = TYPE_NAME(type);
//    writeTripleLinkId(arrayKdmElementId, getLinkIdForType(typeName));


  tree domain = TYPE_DOMAIN(arrayType);
  if (domain)
  {
    tree min = TYPE_MIN_VALUE (domain);
    tree max = TYPE_MAX_VALUE (domain);
    if (min && max && integer_zerop(min) && host_integerp(max, 0))
    {
      std::string nameStr = boost::str(boost::format(HOST_WIDE_INT_PRINT_DEC) % (TREE_INT_CST_LOW (max) + 1));
      writeTriple(arrayKdmElementId, KdmPredicate::Size(), nameStr);
    }
    else
    {
      std::string domainStr;
      if (min)
      {
        domainStr += nodeName(min);
      }
      if (max)
      {
        domainStr += ":" + nodeName(max);
      }
      writeTriple(arrayKdmElementId, KdmPredicate::Size(), domainStr);
    }
  }
  else
  {
    writeTriple(arrayKdmElementId, KdmPredicate::Size(), "<unknown>");
  }
  writeLanguageUnitContains(arrayKdmElementId);
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
long KdmTripleWriter::writeKdmRecordType(tree const recordType, ContainsRelationPolicy const containPolicy, bool isTemplate)
{
  tree mainRecordType = TYPE_MAIN_VARIANT (recordType);

  if (nodeIsMarkedAsProcessed(mainRecordType)) {
    return getReferenceId(mainRecordType);
  }

  if (TREE_CODE(mainRecordType) == ENUMERAL_TYPE)
  {
    std::string msg(str(boost::format("RecordType (%1%) in %2%") % tree_code_name[TREE_CODE(mainRecordType)] % BOOST_CURRENT_FUNCTION));
    writeUnsupportedComment(msg);
    //enum
    return invalidId;
  }
  else if (global_namespace && TYPE_LANG_SPECIFIC (mainRecordType) && CLASSTYPE_DECLARED_CLASS (mainRecordType))
  {
    return writeKdmClassType(mainRecordType, containPolicy, isTemplate);
  }
  else //Record or Union
  {
    markNodeAsProcessed(mainRecordType);

    long compilationUnitId(getSourceFileReferenceId(mainRecordType));

    //struct
    long structId = getReferenceId(mainRecordType);
    writeTripleKdmType(structId, KdmType::RecordType());
    //check to see if we are an anonymous struct
    std::string name;
    std::string linkId;
    if (isAnonymousStruct(mainRecordType))
    {
      //        //tree type = TYPE_MAIN_VARIANT (mainRecordType);
      //
      //        //tree decl (TYPE_NAME (type));
      //        tree id (DECL_NAME (mainRecordType));
      //        const char* name2 (IDENTIFIER_POINTER (id));
      //        std::cerr << name2 << std::endl;

      name = unnamedStructNode + " : " + boost::lexical_cast<std::string>(unnamedStructNumber++);
      linkId = getLinkIdForType(mainRecordType);
    }
    else
    {
      name = nodeName(mainRecordType);
      linkId = name;
    }

    writeTripleName(structId, name);
    if (!isTemplate)
      writeTripleLinkId(structId, linkId);

    if (COMPLETE_TYPE_P (mainRecordType))
    {
      //Output all non-method declarations in the class
      for (tree d(TYPE_FIELDS(mainRecordType)); d; d = TREE_CHAIN(d))
      {
        switch (TREE_CODE(d))
        {
          case TYPE_DECL:
          {
            if (!DECL_SELF_REFERENCE_P(d))
            {
              processAstNodeInternal(d);
              //              std::string msg(str(boost::format("RecordType (%1%) in %2%") % tree_code_name[TREE_CODE(d)] % BOOST_CURRENT_FUNCTION));
              //              writeUnsupportedComment(msg);
            }
            break;
          }
          case FIELD_DECL:
          {
            if (!DECL_ARTIFICIAL(d))
            {
              processAstNodeInternal(d);
            }
            break;
          }
          case VAR_DECL:
          {
            processAstNodeInternal(d);
            break;
          }
          case TEMPLATE_DECL:
          {
            processAstTemplateDecl(d);
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

    //    if (DECL_ARTIFICIAL(mainRecordType))
    //    {
    //      std::cerr << "ARTIFICIAL" << std::endl;
    //    }
    //    else
    //    {
    //      std::cerr << " NOT ARTIFICIAL" << std::endl;
    //    }

    writeKdmSourceRef(structId, mainRecordType);

    if (containPolicy == WriteKdmContainsRelation)
      writeTripleContains(compilationUnitId, structId);

    return structId;
  }
}

#if 1 //BBBB
void KdmTripleWriter::writeKdmFriends(long const id, tree const scope)
{
  //	static int
  //	friend_accessible_p (tree scope, tree decl, tree binfo)

  tree befriending_classes;

  if (!scope)
    return;

  if (TREE_CODE (scope) == FUNCTION_DECL || DECL_FUNCTION_TEMPLATE_P (scope))
    befriending_classes = DECL_BEFRIENDING_CLASSES (scope);
  else if (TYPE_P (scope))
    befriending_classes = CLASSTYPE_BEFRIENDING_CLASSES (scope);
  else
    return;

  for (tree frnd = befriending_classes; frnd; frnd = TREE_CHAIN (frnd))
  {
    if (TREE_CODE (frnd) != FUNCTION_DECL)
    {
      tree tv = TREE_VALUE (frnd);
      if (tv != NULL && TREE_CODE (tv) != TEMPLATE_DECL)
      {
        tree friendType = TREE_VALUE (frnd);
        long friendId = getReferenceId(friendType);
        long relId = writeKdmFriend(id, friendId);
        writeTripleContains(id, relId);
      }
    }
  }

  if (TREE_CODE (scope) == FUNCTION_DECL || DECL_FUNCTION_TEMPLATE_P (scope))
  {
    /* Perhaps this SCOPE is a member of a class which is a friend.  */
    if (DECL_CLASS_SCOPE_P (scope))
    {
      writeKdmFriends(id, DECL_CONTEXT (scope));
    }
  }
}

#else

void KdmTripleWriter::writeKdmFriends(long const id, tree const befriending)
{

#if 1
  for (tree frnd = befriending; frnd; frnd = TREE_CHAIN (frnd))
  {
    tree tv = TREE_VALUE (frnd);
    if (tv != NULL && TREE_CODE (tv) != TEMPLATE_DECL)
    {
      tree friendType = TREE_VALUE (frnd);
      long friendId = getReferenceId(friendType);
      long relId = writeKdmFriend(id, friendId);
      writeTripleContains(id, relId);
    }
  }
#endif

}

#endif

/**
 *
 */
long KdmTripleWriter::writeKdmClassType(tree const recordType, ContainsRelationPolicy const containPolicy, bool isTemplate)
{
  tree mainRecordType = recordType;

  markNodeAsProcessed(mainRecordType);

  long compilationUnitId(getSourceFileReferenceId(mainRecordType));

  long classId = getReferenceId(mainRecordType);
  writeTripleKdmType(classId, KdmType::ClassUnit());

  std::string name;
  if (isAnonymousStruct(mainRecordType)) {
    name = unnamedClassNode + " : " + boost::lexical_cast<std::string>(unnamedStructNumber++);
  } else {
    name = nodeName(mainRecordType);
  }
  writeTripleName(classId, name);

  if (!isTemplate) {
    // link:id is the mangled name, hopefully
    writeTripleLinkId(classId, gcckdm::getLinkId(TYPE_NAME(recordType), name));
  }

  // Is this an abstract class?
  if (CLASSTYPE_PURE_VIRTUALS (recordType) != 0) {
    writeTriple(classId, KdmPredicate::Stereotype(), KdmElementId_AbstractStereotype);
  }

  if (!COMPLETE_TYPE_P (recordType)) {
    writeTriple(classId, KdmPredicate::Stereotype(), KdmElementId_IncompleteStereotype);
  }
  // Hide artificial classes
  //  if (DECL_ARTIFICIAL (recordType))
  //    writeTriple(classId, KdmPredicate::Stereotype(), KdmElementId_HiddenStereotype);

  // Base class information
  // See http://codesynthesis.com/~boris/blog/2010/05/17/parsing-cxx-with-gcc-plugin-part-3/
  tree biv = TYPE_BINFO (recordType);
  size_t n = biv ? BINFO_N_BASE_BINFOS (biv) : 0;

  for (size_t i(0); i < n; i++) {

    tree bi = BINFO_BASE_BINFO (biv, i);

    tree b_type = TYPE_MAIN_VARIANT (BINFO_TYPE (bi));
    //    tree b_decl (TYPE_NAME (b_type));
    //    tree b_id (DECL_NAME (b_decl));

    long superClassId = getReferenceId(b_type);

    long extendsId = writeKdmExtends(classId, superClassId);

    // Add some Stereotypes to the relationship
    // Virtual subclass?
    bool virt = BINFO_VIRTUAL_P (bi);
    if (virt) {
      writeTriple(extendsId, KdmPredicate::Stereotype(), KdmElementId_VirtualStereotype);
    }

    // Access specifier.
    if (BINFO_BASE_ACCESSES (biv)) {
      tree ac = BINFO_BASE_ACCESS (biv, i);

      if (ac == 0 || ac == access_public_node)
        writeTriple(extendsId, KdmPredicate::Stereotype(), KdmElementId_PublicStereotype);
      else if (ac == access_protected_node)
        writeTriple(extendsId, KdmPredicate::Stereotype(), KdmElementId_ProtectedStereotype);
      else
        writeTriple(extendsId, KdmPredicate::Stereotype(), KdmElementId_PrivateStereotype);

    } else {
      writeTriple(extendsId, KdmPredicate::Stereotype(), KdmElementId_PublicStereotype);
    }
    // Write the containment
    writeTripleContains(classId, extendsId);
  }

  // Friends
#if 1 //BBBBB
  writeKdmFriends(classId, recordType);
#else
  writeKdmFriends(classId, CLASSTYPE_BEFRIENDING_CLASSES(recordType));
#endif

  // Traverse members.
  for (tree d = TYPE_FIELDS(mainRecordType); d != 0; d = TREE_CHAIN(d)) {
    switch (TREE_CODE (d)) {
      case TYPE_DECL:
      {
        if (!DECL_SELF_REFERENCE_P (d) && !DECL_IMPLICIT_TYPEDEF_P (d)) {
          processAstNodeInternal(d);
        }
        break;
      }
      case FIELD_DECL:
      {
        if (!DECL_ARTIFICIAL (d)) {
          getReferenceId(d);
          processAstNode(d);
          //BBBB          writeTripleContains(classId, itemId);
        }
        break;
      }
      default:
      {
        processAstNodeInternal(d);
        break;
      }
    }
  }

  for (tree d = TYPE_METHODS (mainRecordType); d != 0; d = TREE_CHAIN (d)) {
    if (!DECL_ARTIFICIAL (d)) {
      processAstNodeInternal(d, WriteKdmContainsRelation, isTemplate);
    }
  }

  writeKdmSourceRef(classId, mainRecordType);

  if (containPolicy == WriteKdmContainsRelation) {
    writeTripleContains(compilationUnitId, classId);
  }

  return classId;
}

void KdmTripleWriter::writeKdmSharedUnit(tree const file)
{
  long id = getSharedUnitReferenceId(file);
  std::string fname = IDENTIFIER_POINTER(file);
  fixPathString(fname);
  writeKdmSharedUnit(Path(fname), id);
}

void KdmTripleWriter::writeKdmSharedUnit(Path const & file, const long id)
{
  Path compFile = checkPathType(mSettings, file);
  writeTripleKdmType(id, KdmType::SharedUnit());
  writeTripleName(id, compFile.filename());
  writeTripleLinkId(id, compFile.string());
  writeTripleContains(getPackageId(compFile.parent_path()), id);
}

long KdmTripleWriter::writeKdmSourceRef(long id, const tree node)
{
  return writeKdmSourceRef(id, expand_location(locationOf(node)));
}

long KdmTripleWriter::writeKdmSourceRef(long id, const expanded_location & eloc)
{
  if (!eloc.file)
  {
    return id;
  }
  std::string fname(eloc.file);
  fixPathString(fname);
  Path sourceFile(fname);

  sourceFile = checkPathType(mSettings, sourceFile);

  FileMap::iterator i = mInventoryMap.find(sourceFile);
  if (i == mInventoryMap.end())
  {
    i = writeKdmSourceFile(sourceFile);
  }
  std::string srcRef = boost::lexical_cast<std::string>(i->second) + ";" + boost::lexical_cast<std::string>(eloc.line);
  writeTriple(id, KdmPredicate::SourceRef(), srcRef);

  return id;
}


void KdmTripleWriter::writeLanguageUnitContains(long const child, bool uid)
{
  //Additions to the language unit are always unlocked.  Additions to the
  //language unit can come from gimple or elsewhere... so we have to
  //ensure that they get proper UID's
  bool val = lockUid();
  lockUid(false);
  writeTripleContains(KdmElementId_LanguageUnit, child, uid);
  lockUid(val);
}


long KdmTripleWriter::writeRelation(KdmType const & type, long const fromId, long const toId)
{
  long relId = getNextElementId();
  writeTripleKdmType(relId, type);
  writeTriple(relId, KdmPredicate::From(), fromId);
  writeTriple(relId, KdmPredicate::To(), toId);
  writeTripleContains(fromId, relId, false); //relations don't have uid's
  return relId;
}

} // namespace kdmtriplewriter

} // namespace gcckdm
