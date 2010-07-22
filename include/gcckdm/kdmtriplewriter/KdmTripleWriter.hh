//
// Copyright (c) 2010 KDM Analytics, Inc. All rights reserved.
// Date: Jun 21, 2010
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
//

#ifndef GCCKDM_KDMTRIPLEWRITER_KDMTRIPLEWRITER_HH_
#define GCCKDM_KDMTRIPLEWRITER_KDMTRIPLEWRITER_HH_

#include <iostream>
#include <queue>
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#include <boost/shared_ptr.hpp>
#include <gcckdm/utilities/unique_ptr.hpp>

#include "gcckdm/GccKdmConfig.hh"
#include "gcckdm/GccAstListener.hh"
#include "gcckdm/GccKdmUtilities.hh"
#include "gcckdm/KdmPredicate.hh"
#include "gcckdm/KdmType.hh"
#include "gcckdm/KdmKind.hh"
#include "gcckdm/kdmtriplewriter/PathHash.hh"
#include "gcckdm/kdmtriplewriter/TripleWriter.hh"

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
  static const int KdmTripleVersion = 1;

  /**
   * Pointer to the output stream this writer uses to create output
   */
  typedef boost::shared_ptr<std::ostream> KdmSinkPtr;

  /**
   * Constructs a KdmTripleWriter which directs it's output to the stream
   * in the given pointer
   *
   * @param kdmSink pointer to a output stream
   */
  explicit KdmTripleWriter(KdmSinkPtr const & kdmSink);

  /**
   * Construct a KdmTriplewriter which directs it's output to the file
   * with the given filename
   *
   * @param filename the file contain the kdm output
   */
  explicit KdmTripleWriter(Path const & filename);

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
   * Convenience method to write the common "contains" triple
   *
   * writes: <subject> <contains> <child>
   */
  void writeTripleContains(long const parent, long const child);

  /**
   * Convenience method to write the common "LinkId" triple
   *
   * writes: <subject> <linkId> <child>
   */
  void writeTripleLinkId(long const subject, std::string const & name);

  /**
   * Convenience method to write the common "kind" triple
   *
   * writes: <subject> <kind> <kind>
   */
  void writeTripleKind(long const subject, KdmKind const & kind);

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
   * Returns true if the given node has already been encountered and
   * can be referenced
   *
   * @param node the node to test for a reference id
   */
  bool hasReferenceId(tree const node) const;

  /**
   * Returns the id for the given node.  If the node
   * doesn't already have a id, the next available element id is inserted
   * into the referenceNode map and the node is placed in the process queue
   *
   * Note: this method can cause the elementId counter to increase
   */
  long getReferenceId(tree const node);

  /**
   * Returns the next available element Id.
   *
   * Note: Calling this method causes the element id counter to increase
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
   * Returns true if this writer decends into function/method bodies
   * to process GIMPLE statements
   */
  bool bodies() const;

  /**
   * Enables or disables the decent into function/method bodies
   *
   * @param value if true this writer will process GIMPLE statements
   */
  void bodies(bool value);

private:

  typedef std::tr1::unordered_map<tree, long> TreeMap;
  typedef std::tr1::unordered_map<Path, long> FileMap;
  typedef std::tr1::unordered_set<tree> TreeSet;
  typedef std::queue<tree> TreeQueue;
  typedef boost::unique_ptr<GimpleKdmTripleWriter> GimpleWriter;

  long getSourceFileReferenceId(tree const decl);
  long getSharedUnitReferenceId(tree const file);

  enum
  {
    KdmElementId_Segment = 0,
    KdmElementId_CodeModel,
    KdmElementId_WorkbenchExtensionFamily,
    KdmElementId_HiddenStereoType,
    KdmElementId_CodeAssembly,
    KdmElementId_LanguageUnit,
    KdmElementId_DerivedSharedUnit,
    KdmElementId_ClassSharedUnit,
    KdmElementId_InventoryModel,
    KdmElementId_CompilationUnit,
    KdmElementId_DefaultStart,
  };

  void processAstDeclarationNode(tree const decl);
  void processAstTypeNode(tree const decl);
  void processAstFunctionDeclarationNode(tree const functionDecl);
  void processAstFieldDeclarationNode(tree const fieldDecl);
  void processAstVariableDeclarationNode(tree const varDecl);
  void processAstValueNode(tree const valueConst);
//  void processAstLabelDeclarationNode(tree const labelDecl);
  void writeVersionHeader();
  void writeDefaultKdmModelElements();

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
  void writeKdmSourceFile(Path const & file);
  void writeKdmCompilationUnit(Path const & file);
  void writeKdmCallableUnit(tree const functionDecl);
  long writeKdmReturnParameterUnit(tree const param);
  long writeKdmParameterUnit(tree const param);
  void writeKdmPrimitiveType(tree const type);
  void writeKdmPointerType(tree const type);

  /**
   * Handles output of enums, classes, and structs
   */
  void writeKdmRecordType(tree const type);

  void writeKdmSharedUnit(tree const file);
  long writeKdmItemUnit(tree const item);
  void writeKdmArrayType(tree const array);
  long writeKdmStorableUnit(tree const var);
  long writeKdmSignature(tree const function);
  long writeKdmSignatureDeclaration(tree const functionDecl);
  long writeKdmSignatureType(tree const functionType);
  long writeKdmSourceRef(long id, tree const var);
  long writeKdmValue(tree const val);

  KdmSinkPtr mKdmSink; /// Pointer to the kdm output stream
  long mKdmElementId; /// The current element id, incremented for each new element
  GimpleWriter mGimpleWriter;
  TreeMap mReferencedNodes;
  TreeMap mReferencedSharedUnits;
  Path mCompilationFile;
  FileMap mInventoryMap;
  TreeSet mProcessedNodes;
  TreeQueue mNodeQueue;

  bool mBodies;
};

} // namespace kdmtriplewriter

} // namespace gcckdm

#endif /* GCCKDM_KDMTRIPLEWRITER_KDMTRIPLEWRITER_HH_ */
