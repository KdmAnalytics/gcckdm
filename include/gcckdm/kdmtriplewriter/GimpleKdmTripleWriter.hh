//
// Copyright (c) 2010 KDM Analytics, Inc. All rights reserved.
// Date: Jul 13, 2010
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

#ifndef GCCKDM_KDMTRIPLEWRITER_GIMPLEKDMTRIPLEWRITER_HH_
#define GCCKDM_KDMTRIPLEWRITER_GIMPLEKDMTRIPLEWRITER_HH_

#include <tr1/unordered_map>
#include "gcckdm/GccKdmConfig.hh"
#include "gcckdm/GccKdmUtilities.hh"

#include "gcckdm/kdmtriplewriter/KdmTripleWriterFwd.hh"
#include "gcckdm/kdmtriplewriter/ExpandedLocationFunctors.hh"

namespace gcckdm
{

namespace kdmtriplewriter
{

/**
 * Component of the KdmTripleWriter that handles GimpleStructures
 */
class GimpleKdmTripleWriter
{
public:

  /**
   * Constructs a kdm triple writer that knows about GIMPLE
   * how to parse it, how to convert GIMPLE to KDM
   *
   * @param kdmTripleWriter the triple writer to use to write KDM
   */
  GimpleKdmTripleWriter(KdmTripleWriter & kdmTripleWriter);

  /**
   * Destructor
   */
  virtual ~GimpleKdmTripleWriter();

  /**
   * Checks to ensure that the functionDecl has a GIMPLE body and
   * then iterates through the gimple statements converting them
   * to eqivalent KDM statements
   *
   * @param functionDecl the function declaration containing GIMPLE body to convert to KDM
   */
  void processAstFunctionDeclarationNode(tree functionDecl);

private:
  typedef std::tr1::unordered_map<expanded_location, long, ExpanedLocationHash, ExpandedLocationEqual> LocationMap;

  /**
   * Iterates through all gimple statements in the given gimple_seq and
   * calls processGimpleStatement for each statement.
   *
   * This is the function to call to start the GIMPLE to KDM conversion
   *
   * @param parent the AST node which contains this gimple_seq ie a function_decl
   * @param gs a sequence of gimple statements.
   */
  void processGimpleSequence(gimple_seq const gs);

  /**
   * Determines the type of the given gimple statement and calls the appropriate
   * method to handle that type of statement.
   *
   *
   * @param parent the AST node which contains this gimple_seq ie a function_decl
   * @param gs a gimple statement.
   */
  void processGimpleStatement(gimple const gs);


  /**
   * Return the id of the Block unit for the given location.
   *
   * @param functionDecl the function declaration used to determine the parent of the block unit
   * @param loc the location structure to determin the block unit for this line
   */
  long getBlockReferenceId(location_t const loc);

  /**
   * Return the id of the Block unit for the given location
   *
   * @param callableUnitId the id of the callableUnit that contains the given location
   * @param loc the location information to use to retrieve a blockUnit id
   */
  long getBlockReferenceId(long const callableUnitId, location_t const loc);

  void processGimpleBindStatement(gimple const gs);
  long processGimpleAssignStatement(gimple const gs);
  long processGimpleReturnStatement(gimple const gs);
  long processGimpleConditionalStatement(gimple const gs);
  long processGimpleLabelStatement(gimple const gs);
  long processGimpleCallStatement(gimple const gs);
  long processGimpleGotoStatement(gimple const gs);
  void processGimpleSwitchStatement(gimple const gs);

  void processGimpleUnaryAssignStatement(long const actionId, gimple const gs);
  void processGimpleBinaryAssignStatement(long const actionId, gimple const gs);
  void processGimpleTernaryAssignStatement(long const actionId, gimple const gs);

  long processGimpleAstNode(long const actionId, gimple const gs, tree const ast);

  long writeKdmNopForLabel(tree const label);
  long writeKdmActionRelation(KdmType const & type, long const fromId, long const toId);

  void writeKdmUnaryRelationships(long const actionId, long lhsId, long rhsId);
  void writeKdmBinaryRelationships(long const actionId, long lhsId, long rhs1Id, long rhs2Id);

  void writeKdmUnaryOperation(long const actionId, KdmKind const & kind, gimple const gs);
  void writeKdmBinaryOperation(long const actionId, KdmKind const & kind, gimple const gs);
  void writeKdmArraySelect(long const actionId, gimple const gs);
  void writeKdmPtr(long const actionId, gimple const gs);
  long writeKdmPtrParam(long const actionId, tree const callNode, tree const addrExpr, gimple const gs);
  void writeKdmPtrReplace(long const actionId, gimple const gs);

  long writeKdmStorableUnit(long const typeId, expanded_location const & xloc);

  long getRhsReferenceId(tree const rhs);

  tree resolveCall(tree const tree);

  /// The current AST node containing the gimple statements being processed
  tree mCurrentFunctionDeclarationNode;

  /// The current callable unit id
  long mCurrentCallableUnitId;

  /**
   * Reference to the main kdm triple writer
   */
  KdmTripleWriter & mKdmWriter;

  /**
   * Source line location to block unit id map
   */
  LocationMap mBlockUnitMap;


  bool mLabelFlag;
  long mLastLabelId;
  long mRegisterVariableIndex;
};

} // namespace kdmtriplewriter

} // namespace gcckdm

#endif /* GCCKDM_KDMTRIPLEWRITER_GIMPLEKDMTRIPLEWRITER_HH_ */
