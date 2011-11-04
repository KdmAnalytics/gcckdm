//
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

#ifndef GCCKDM_KDMTRIPLEWRITER_KDMTRIPLEWRITER_HH_
#define GCCKDM_KDMTRIPLEWRITER_KDMTRIPLEWRITER_HH_

#include <iostream>
#include <queue>
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#include <boost/shared_ptr.hpp>

//GCC c-common.h defines a null_node macro, boost::multi_index
//used by boost::graph which causes much breakage we undefine
//it for the duration of the boost include and restore it after
#undef null_node
#include <boost/graph/adjacency_list.hpp>

//restore null_node macro just in case
#define null_node c_global_trees[CTI_NULL]

#include <gcckdm/utilities/unique_ptr.hpp>

#include "gcckdm/GccKdmConfig.hh"
#include "gcckdm/GccAstListener.hh"
#include "gcckdm/GccKdmUtilities.hh"
#include "gcckdm/KdmPredicate.hh"
#include "gcckdm/KdmType.hh"
#include "gcckdm/IKdmKind.hh"
#include "gcckdm/kdmtriplewriter/PathHash.hh"
#include "gcckdm/kdmtriplewriter/TripleWriter.hh"
#include "gcckdm/kdmtriplewriter/UidNode.hh"
#include "gcckdm/kdmtriplewriter/KdmTripleWriterFwd.hh"

namespace gcckdm
{

namespace kdmtriplewriter
{

/**
 * This class traverses the Gcc AST nodes passed to it and writes their KDM
 * representation to and output stream
 */
class KdmTripleWriter: public GccAstListener, public TripleWriter
{
public:

  //Flag to determine if the contains relationship is to be written or not
  enum ContainsRelationPolicy
  {
    WriteKdmContainsRelation = 0,
    SkipKdmContainsRelation = 1
  };

  //Flag to determine if the signature is to be written or not
  enum SignatureUnitPolicy
  {
    WriteSignatureUnit = 0,
    SkipSignatureUnit = 1
  };

  /**
   * User Configurable Writer Settings
   *
   */
  struct Settings
  {
    /**
     * Sets the default values for settings for the writer
     */
    Settings()
      : functionBodies(true),
        generateUids(true),
        generateUidGraph(false),
        containmentCheck(true),
        assemberOutput(false),
        outputExtension(".tkdm"),
        outputFile(""),
        outputDir(""),
        outputGimple(false),
        preprocessed(false),
        outputCompletePath(false),
        outputRegVarNames(false)
    {}

    /// If true function body information written to output
    bool functionBodies;
    /// if true Uids are generated and written to output
    bool generateUids;
    /// if true Uid graph is written to a file called 'graph.viz' viewable in dotty
    bool generateUidGraph;
    /// If true the write will issue a warning when a child is contained in two parents
    bool containmentCheck;
    /// If true generate assembler output, useful for integrating normal compilation
    bool assemberOutput;
    /// The extension on the generated output file
    std::string outputExtension;
    /// The name of the output file
    std::string outputFile;
    ///Output directory all output is placed here, if it's empty the output is place right beside the input file
    KdmTripleWriter::Path outputDir;
    // If true include gimple in generated kdm as a comment
    bool outputGimple;
    // If true source being compiled it assumed to be preprocessed
    bool preprocessed;
    // If true attempts to use complete path when dealing with file locations
    bool outputCompletePath;
    // If true output the name of register variable
    bool outputRegVarNames;

  };

  /**
   * Version of KDM triple file generated by this writer
   */
  static const int KdmTripleVersion = 1;

  static long const invalidId = -1;

  /**
   * Pointer to the output stream this writer uses to create output
   */
  typedef boost::shared_ptr<std::ostream> KdmSinkPtr;

  /**
   * Constructs a KdmTripleWriter which directs it's output to the stream
   * in the given pointer
   *
   * @param kdmSink pointer to a output stream
   * @param settings settings that can be configured from the command line
   */
  explicit KdmTripleWriter(KdmSinkPtr const & kdmSink, Settings const & settings);

  /**
   * Construct a KdmTriplewriter which directs it's output to the file
   * with the given filename
   *
   * @param filename the file contain the kdm output
   * @param settings settings that can be configured from the command line
   */
  explicit KdmTripleWriter(Path const & filename, Settings const & settings);

  /**
   * Destructor
   */
  virtual ~KdmTripleWriter();

  /**
   * @see GccAstListener::startTranslationUnit
   */
  virtual void startTranslationUnit(Path const & file);

  /**
   * @see GccAstListener::startKdmGimplePass
   */
  virtual void startKdmGimplePass();

  /**
   * @see GccAstListener::processAstNode
   */
  virtual void processAstNode(tree const ast);

  /**
   * @see GccAstListener::finishKdmGimplePass
   */
  virtual void finishKdmGimplePass();

  /**
   * @see GccAstListener::finishTranslationUnit
   */
  virtual void finishTranslationUnit();

  /**
   * @see TripleWriter::writerTriple
   */
  virtual void writeTriple(long const subject, KdmPredicate const & predicate, long const object);

  /**
   * @see TripleWriter::writerTriple
   */
  virtual void writeTriple(long const subject, KdmPredicate const & predicate, KdmType const & object);

  /**
   * @see TripleWriter::writerTriple
   */
  virtual void writeTriple(long const subject, KdmPredicate const & predicate, std::string const & object);

  /**
   * Convenience method to write the common "kdmType" triple.
   *
   * writes  <subject> <kdmType> <type>
   *
   * @param subject the subject id
   * @param type
   */
  void writeTripleKdmType(long const subject, KdmType const & type);

  /**
   * Convenience method to write the common "name" triple
   *
   * writes: <subject> <name> <name>
   */
  void writeTripleName(long const subject, std::string const & name);

  /**
   * Convenience method to write the common "pos" triple
   *
   * writes: <subject> <pos> <int>
   */
  void writeTriplePosition(long const id, int pos);

  /**
   * Convenience method to write the common "LinkId" triple
   *
   * writes: <subject> <link:id> <linkId>
   *
   * @param subject
   * @param linkId
   */
  void writeTripleLinkId(long const subject, std::string const & linkId);

  /**
   * Convenience method to write the common "kind" triple
   *
   * writes: <subject> <kind> <kind>
   *
   * @param subject
   * @param kind
   */
  void writeTripleKind(long const subject, IKdmKind const & kind);


  /**
   * Convenience method to write the "export" triple
   *
   * @param subject the refId of the element to attach the export attribute
   * @param exportname the export name to attach to the given subject
   */
  void writeTripleExport(long const subject, std::string const & exportName);


  /**
   * Write storable unit and optionally contain the element with
   * the source file it is found in.
   *
   * @param var the variable decl to write as a StorableUnit
   * @param writeContains if true writes contains triple using the file var is found in as the parent
   * @return the refId of the StorableUnit
   */
  long writeKdmStorableUnit(tree const var, ContainsRelationPolicy const containPolicy);

  void writeKdmStorableUnitKindGlobal(tree const var);
  void writeKdmStorableUnitInitialization(tree const var);

  /**
   * Write the common "extends" relationship
   *
   * writes: <extendsId> <kdmType> "code/Extends"
   * writes: <extendsId> <from> <subclass>
   * writes: <extendsId> <to> <superclass>
   *
   * @param subclass refId of the subclass
   * @param superclass refId of the superclass
   *
   * @return refId of the Extends relationship
   */
  long writeKdmExtends(long const subclass, long const superclass);

  /**
   * Write the common "friend" relationship
   *
   * writes: <extendsId> <kdmType> "code/CodeRelationship"
   * writes: <extendsId> <from> <subclass>
   * writes: <extendsId> <to> <superclass>
   * writes: <extendsId> <Stereotype> <friendId>
   *
   * @param subclass refId of the subclass
   * @param superclass refid of the superclass
   * @return refId of the CodeRelationship
   */
  long writeKdmFriend(long const subclass, long const superclass);

  /**
   * Writes a KDM Source ref using the information contained in the expanded_location
   *
   * Writes: <id> <SourceRef> "<file id>;<line number>
   *
   * Example:  <44> <SourceRef> "15;6"
   *
   * @param id the Id of the element that need a source ref
   * @param xloc
   *
   * @return the id that was passed into the metho
   */
  long writeKdmSourceRef(long id, expanded_location const & xloc);

  /**
   * Add the built-in stereotype to the given subject id
   *
   * @param the id of an element to assign the "built-in" stereotype
   */
  void writeKdmBuiltinStereotype(long const id);

  /**
   * Returns true if the given node has already been encountered and
   * can be referenced
   *
   * @param node the node to test for a reference id
   * @return true if the given node already has a refId
   */
  bool hasReferenceId(tree const node) const;

  /**
   * Returns the id for the given node.  If the node
   * doesn't already have a id, the next available element id is inserted
   * into the referenceNode map and the node is placed in the process queue
   *
   * Note: this method can cause the elementId counter to increase
   *
   * @return a refId for the given node
   */
  long getReferenceId(tree const node);

  long getId(tree const node);
  long getId_orInvalidIdForNULL(tree const node);

  /**
   * Returns the next available element Id.
   *
   * Note: Calling this method causes the element id counter to increase
   *
   * @return a refId;
   */
  long getNextElementId();

  /**
   * Writes the given string as comments.  Multiline strings are written as
   * multiple comments
   *
   * @param comment the comment to be inserted into the output stream
   */
  void writeComment(std::string const & comment);

  /**
   * Writes <code>comment</code as a comment with the UNSUPPORTED prefix
   *
   * @param comment the string to write to output stream
   */
  void writeUnsupportedComment(std::string const & comment);

  /**
   * Some types are specifically represented by code for example
   * BooleanType... we insert these types when required and allow
   * them to be referenced.  Used by the gimple part of the writer
   *
   * @param type
   * @return the refId for the given type
   */
  long getUserTypeId(KdmType const & type);

  /**
   * In certain cases some nodes are processed outside the regular
   * flow of processAstNode.  In those cases you can mark the node
   * as processed using this method.  This prevents the given
   * node from double containment errors and other processing
   * problems
   *
   * @param node the ast node to add to the processed queue
   */
  void markNodeAsProcessed(tree node);

  bool nodeIsMarkedAsProcessed(tree const ast);


  /**
   * Return an id for the given value node.  If a node
   * with the same name exists already in the language unit
   * that value is returned instead
   *
   * @param node a value node ie string, integer, etc
   */
  long getValueId(tree const node);

  long writeKdmValue(tree const val);

  /**
   * Write relation of the given type from fromId to toId
   *
   * @param type the KDM type of the relation to write
   * @param fromId the id of the origin of the relation
   * @param toId the id of the target of the relation
   */
  long writeRelation(KdmType const & type, const long fromId, const long toId);

  long writeKdmParameterUnit(tree const param, bool forceNewElementId = false);

  /**
   * Convenience method to write the common "contains" triple
   *
   * writes: <subject> <contains> <child>
   *
   * @param parent
   * @param child
   * @param uid if true adds the given contains relationship to the uid graph
   */
  void writeTripleContains(long const parent, long const child, bool uid = true);

  /**
   * Enum of all constant subject id's
   */
  enum ReservedElementId
  {
    KdmElementId_Segment = 0,
#if 1 //BBBB
    KdmElementId_Audit,
#endif
    KdmElementId_CodeModel,
    KdmElementId_WorkbenchExtensionFamily,
    KdmElementId_HiddenStereotype,
    KdmElementId_CxxExtensionFamily,
    KdmElementId_MutableStereotype,
    KdmElementId_VolatileStereotype,
    KdmElementId_ConstStereotype,
    KdmElementId_StaticStereotype,
    KdmElementId_InlineStereotype,
    KdmElementId_RestrictStereotype,
    KdmElementId_VirtualStereotype,
    KdmElementId_PureVirtualStereotype,
    KdmElementId_AbstractStereotype,
    KdmElementId_FriendStereotype,
    KdmElementId_IncompleteStereotype,
    KdmElementId_PublicStereotype,
    KdmElementId_PrivateStereotype,
    KdmElementId_ProtectedStereotype,
    KdmElementId_ExplicitStereotype,
    KdmElementId_BuiltinStereotype,
    KdmElementId_CodeAssembly,
    KdmElementId_LanguageUnit,
#if 0 //BBBB
    KdmElementId_DerivedSharedUnit,
    KdmElementId_ClassSharedUnit,
#endif
    KdmElementId_InventoryModel,
    KdmElementId_CompilationUnit,
    KdmElementId_DefaultStart
  };


private:

  /**
   * Functor for hashing KdmType's in a unordered_map
   *
   * Currently uses the integer that represents the KdmType and
   * the standard hash
   */
  struct KdmTypeHash
  {
    size_t operator()(KdmType const & v) const
    {
      std::tr1::hash<int> h;
      return h(v.id());
    }
  };

  typedef std::tr1::unordered_map<std::string, long> ValueMap;
  typedef std::tr1::unordered_map<tree, long> TreeMap;
  typedef std::tr1::unordered_map<Path, long> FileMap;
  typedef std::tr1::unordered_set<tree> TreeSet;
  typedef std::queue<tree> TreeQueue;
  typedef boost::unique_ptr<GimpleKdmTripleWriter> GimpleWriter;
  typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, UidNode> UidGraph;
  typedef boost::graph_traits<UidGraph>::vertex_descriptor Vertex;
  typedef std::tr1::unordered_map<long, long> ContainmentMap;
  typedef std::tr1::unordered_map<KdmType, long, KdmTypeHash> TypeMap;

  /**
   * Returns the id for source file containing the given node
   *
   * @param node
   */
  long getSourceFileReferenceId(tree const node);

  /**
   * Returns the shared unit id for the given identifierNode
   *
   * Note: this should only be called by getSourceFileReferenceId
   */
  long getSharedUnitReferenceId(tree const identifier);

  /**
   * Version of processAstNode to be used inside the triple writer.  Adds default variables
   *
   * @param ast the ast node to process
   * @param containPolicy the policy to follow when writing the contains relationship
   * @param isTemplate true is the node being processed is a template node.
   *
   * @return the id assigned to the node being processed, to be used to reference the processed node
   */
  long processAstNodeInternal(tree const ast, ContainsRelationPolicy const containPolicy = WriteKdmContainsRelation, bool isTemplate = false);

  /**
   * Process the AST Constructor node
   */
  long processAstDeclarationNode(tree const decl, ContainsRelationPolicy const containPolicy = WriteKdmContainsRelation, bool isTemplate = false);

  /**
   * Convert the given FUNCTION_DECL node to the equivalent KDM
   * CallableUnit.  Equvalent to calling writeKdmCallableUnit
   *
   * @see writeKdmCallableUnit
   * @param functionDecl a FUNCTION_DECL to convert to CallableUnit
   */
  void processAstFunctionDeclarationNode(tree const functionDecl, ContainsRelationPolicy const containPolicy = WriteKdmContainsRelation, bool isTemplate = false);

  /**
   * Convert a FIELD_DECL into an KDM ItemUnit and write the result
   * to the configured kdmSink.  If the source being compiled is
   * C++ the FIELD_DECL can also be converted to a Kdm MemberUnit
   *
   * @param fieldDecl ast node to output as to ItemUnit or MemberUnit
   */
  void processAstFieldDeclarationNode(tree const fieldDecl);

  /**
   * Iterates through all nodes contained within the namespace and
   * processes them by passing them to processAstNode
   *
   * @see processAstNode
   * @param namespaceNode a namespace AST node
   */
  void processAstNamespaceNode(tree const namespaceNode);

  /**
   * Determines the actual type of the given typeNode and writes the
   * KDM representation of the type to the configured kdmSink using
   * one the writeAst*TypeNode methods
   *
   * @param typeNode the AST type node to convert to KDM
   */
  long processAstTypeNode(tree const typeNode, ContainsRelationPolicy const containPolicy = WriteKdmContainsRelation, bool isTemplate = false, const long containedInId = invalidId);

  /**
   * Determines if the given recordTypeNode is suitable for KDM representation
   * Some recordTypeNodes are not required to be represented in KDM
   *
   * @param recordTypeNode
   */
  long processAstRecordTypeNode(tree const recordTypeNode);

  /**
   * Iterates through all template specializations and instantiations
   * and processes all the AST node using processAstNode
   *
   * @see processAstNode
   * @param templateDecl the template declaration
   */
  void processAstTemplateDecl(tree const templateDecl);

  /**
   * A type declaration may be a typedef, a new class, or an enumeration
   */
  long processAstTypeDecl(tree const typeNode);
  void processAstVariableDeclarationNode(tree const varDecl);
  long processAstValueNode(tree const valueConst, ContainsRelationPolicy const containPolicy = WriteKdmContainsRelation);

  void writeVersionHeader();

  void writeDefaultKdmModelElements();


  void writeLanguageUnitContains(long const child, bool uid = true);

  /**
   * Write a SourceFile kdm element to the KdmSink stream using the given file
   *
   * Sample output:
   *
   * <10> <kdmtype> "source/SourceFile".
   * <10> <name> "test002.c".
   * <10> <path> "/tmp/c-tests/test002.c".
   * <10> <link::id> "/tmp/c-tests/test002.c".
   * <8> <contains> <10>.
   *
   * @param file the file to use to populate the SourceFile kdm element
   */
  FileMap::iterator writeKdmSourceFile(Path const & file);
  void writeKdmCompilationUnit(Path const & file);

  /**
   * Write a CallableUnit kdm element to the KdmSink stream using the
   * given 'callable' delcaration currently supported 'callables'
   *
   *   Function
   *   Method
   *     Destructor
   *     Constructor
   *     Operator
   *
   * @param callableDecl an ast node of a function or method
   */
  long writeKdmCallableUnit(tree const callableDecl, ContainsRelationPolicy const containPolicy = WriteKdmContainsRelation, bool isTemplate = false);

  /**
   * Write a CallableUnit kdm element to the KdmSink stream using the
   * given 'callable' delcaration currently supported 'callables'
   *
   *   Function
   *   Method
   *     Destructor
   *     Constructor
   *     Operator
   *
   *  Allows control of the callableDeclId and if the signature is to be written for this callable or not.
   *
   * @param callableDecl an ast node of a function or method
   */
  long writeKdmCallableUnit(long const callableDeclId, tree const callableDecl, ContainsRelationPolicy const containPolicy = WriteKdmContainsRelation, bool isTemplate = false, SignatureUnitPolicy const sigUnitPolicy = WriteSignatureUnit);

  /**
   * In C++ containment of various elements depends on whether the definition's "context" is
   * a class or not. This method determines containment in these more complex situations.
   */
  void writeKdmCxxContains(long const declId, tree const decl);

  long writeKdmReturnParameterUnit(tree const param);
  void writeKdmPrimitiveType(tree const type);
  long writeKdmPointerType(tree const pointerType, ContainsRelationPolicy const containPolicy = WriteKdmContainsRelation, bool isTemplate = false, const long containedInId = invalidId);
  void writeEnumType(tree const enumType);

public:
  tree getTypeNode(tree type);

  /**
    * Returns the item unit contained within the array type with the given id.  If an 
    * an array type with the given id does not exist creates a new id and adds it to the 
    * lookup map
    *
    * @param arrayTypeId the id of an ArrayType
    * @return the item unit from the array type with the given id
    */
  long getItemUnitId(long arrayTypeId);

private:
  /**
   * Handles output of enums, and structs, pass through of class to writeKdmClassType
   */
  long writeKdmRecordType(tree const type, ContainsRelationPolicy const containPolicy = WriteKdmContainsRelation, bool isTemplate = false);

  /**
   * Handles output of classes
   */
  long writeKdmClassType(tree const recordType, ContainsRelationPolicy const containPolicy = WriteKdmContainsRelation, bool isTemplate = false);

  /**
   *
   */
  void writeKdmMethodType(tree const type);

  /**
   * Handles output of friend relationships, which are stereotyped CodeRelationships
   */
  void writeKdmFriends(long const id, tree const t);

  void writeKdmSharedUnit(tree const file);
  void writeKdmSharedUnit(Path const & filename, long const id);

  /**
   * Output the MemberUnit definition.
   */
  long writeKdmMemberUnit(tree const item);
  long writeKdmItemUnit(tree const item);
  void writeKdmArrayType(tree const arrayType);

  long writeKdmSignature(tree const function);
  long writeKdmSignatureDeclaration(tree const functionDecl);

  /**
   * Write a variable that represents a value that is thrown
   */
  long writeKdmThrows(long const id);

  long writeKdmSignatureType(tree const functionType);
  long writeKdmSourceRef(long id, tree const var);

  /**
   * Write type qualifiers as Stereotypes on the declarations
   */
  void writeKdmTypeQualifiers(tree const decl);

  void processNodeQueue();
  void writeReferencedSharedUnits();
  void writeUids();

  long getPackageId(Path const & path);
  long getDirectoryId(Path const & path);
  long getLocationContextId(Path const & path, long const rootId, FileMap & fMap, KdmType const & type);

  int find_template_parm (tree t);
  /**
   * Allow or prevent the increase in the UID when a contains relationship is called.
   * Used to set everything below the CallableUnit to have the same UIDs
   */
  void lockUid(bool val);

  /**
   * Returns true if the the current value of the UID is locked otherwise it returns true
   *
   * @return state of the UID lock
   */
  bool lockUid() const;

  /**
   * Adds nodes for the given id's if they don't already exist
   * and adds the relationship between them.
   *
   * @returns true if the graph was updated (no duplicate containment)
   */
  bool updateUidGraph(const long parent, const long child);

  KdmSinkPtr mKdmSink; /// Pointer to the kdm output stream
  long mKdmElementId; /// The current element id, incremented for each new element
  GimpleWriter mGimpleWriter; /// The gimple writer
  TreeMap mReferencedNodes;
  TreeMap mReferencedSharedUnits;

  /// The complete (absolute) path to the file being compiled
  Path mCompilationFile;
  FileMap mInventoryMap;
  TreeSet mProcessedNodes;
  TreeQueue mNodeQueue;

  //Graph to store generated UID's
  UidGraph mUidGraph;

  //Allow UidVisitor functor access to private members
  friend class UidVisitor;
  class UidVisitor;

  /// The currrent uid.  Incremented for each new contains relationship
  long mUid;

  TypeMap mUserTypes;

  ///List of element elements.  Used for duplicate contains detection
  ContainmentMap mContainment;

  ///Map of ArrayType ids to ItemUnit ids.  Used for relationships to array elements
  ContainmentMap mItemUnits;

  ///User configuration settings
  Settings mSettings;

  FileMap mPackageMap;
  FileMap mDirectoryMap;

  bool mLockUid;
};

} // namespace kdmtriplewriter

} // namespace gcckdm

#endif /* GCCKDM_KDMTRIPLEWRITER_KDMTRIPLEWRITER_HH_ */
