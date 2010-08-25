//
// Copyright (c) 2010 KDM Analytics, Inc. All rights reserved.
// Date: Jul 13, 2010
// Author: Kyle Girard <kyle@kdmanalytics.com>
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

#ifndef GCCKDM_KDMTRIPLEWRITER_GIMPLEKDMTRIPLEWRITER_HH_
#define GCCKDM_KDMTRIPLEWRITER_GIMPLEKDMTRIPLEWRITER_HH_

#include <tr1/unordered_map>
#include "gcckdm/GccKdmConfig.hh"
#include "gcckdm/GccKdmUtilities.hh"

#include "gcckdm/kdmtriplewriter/KdmTripleWriterFwd.hh"
#include "gcckdm/kdmtriplewriter/ExpandedLocationFunctors.hh"
#include "gcckdm/kdmtriplewriter/ActionData.hh"

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
  typedef boost::shared_ptr<ActionData> ActionDataPtr;
  //typedef boost::shared_ptr<Flow> ActionDataPtr;
  //typedef boost::shared_ptr<ActionFlow> ActionActionDataPtr;


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


  /**
   * Taken from GCC Doc's....
   *
   *  GIMPLE_BIND <VARS, BLOCK, BODY> represents a lexical scope.
   *  VARS is the set of variables declared in that scope.
   *  BLOCK is the symbol binding block used for debug information.
   *  BODY is the sequence of statements in the scope.
   *
   *  We iterate through each variable in VARS and send those
   *  variables to be processed by the KDM writer
   *
   *  And then process all statements in the BODY by sendin the
   *  sequence to processGimpleSequence
   *
   *  @param gs the gimple bind statement to be processed
   */
  void processGimpleBindStatement(gimple const gs);

  /**
   * Info taken from GCC Doc's....
   *
   * GIMPLE_ASSIGN <SUBCODE, LHS, RHS1[, RHS2]> represents the assignment
   * statement
   *
   * LHS = RHS1 SUBCODE RHS2.
   *
   * SUBCODE is the tree code for the expression computed by the RHS of the
   * assignment.  It must be one of the tree codes accepted by
   * get_gimple_rhs_class.  If LHS is not a gimple register according to
   * is_gimple_reg, SUBCODE must be of class GIMPLE_SINGLE_RHS.
   *
   * LHS is the operand on the LHS of the assignment.  It must be a tree node
   * accepted by is_gimple_lvalue.
   *
   * RHS1 is the first operand on the RHS of the assignment.  It must always be
   * present.  It must be a tree node accepted by is_gimple_val.
   *
   * RHS2 is the second operand on the RHS of the assignment.  It must be a tree
   * node accepted by is_gimple_val.  This argument exists only if SUBCODE is
   * of class GIMPLE_BINARY_RHS.
   *
   *
   * Calling this method reserves an subject id, writes an action element triple
   * and passes that actionElement id to all methods called by this method
   * to handle the individual constructs.
   *
   * The containment of the generated action element is also written to the
   * sink.
   *
   */
  ActionDataPtr processGimpleAssignStatement(gimple const gs);


  ActionDataPtr processGimpleReturnStatement(gimple const gs);
  ActionDataPtr processGimpleConditionalStatement(gimple const gs);
  ActionDataPtr processGimpleLabelStatement(gimple const gs);
  ActionDataPtr processGimpleCallStatement(gimple const gs);
  ActionDataPtr processGimpleGotoStatement(gimple const gs);
  ActionDataPtr processGimpleSwitchStatement(gimple const gs);

  ActionDataPtr processGimpleUnaryAssignStatement(gimple const gs);
  ActionDataPtr processGimpleBinaryAssignStatement(gimple const gs);
  ActionDataPtr processGimpleTernaryAssignStatement(gimple const gs);

  long processGimpleAstNode(long const actionId, gimple const gs, tree const ast);

  ActionDataPtr writeKdmNopForLabel(tree const label);
  long writeKdmActionRelation(KdmType const & type, long const fromId, long const toId);
  long writeKdmFlow(long const fromId, long const toId);

  void writeKdmUnaryRelationships(long const actionId, long lhsId, long rhsId);
  void writeKdmBinaryRelationships(long const actionId, long lhsId, long rhs1Id, long rhs2Id);

  ActionDataPtr writeKdmUnaryOperation(KdmKind const & kind, tree const lhs, tree const rhs);
  ActionDataPtr writeKdmUnaryOperation(KdmKind const & kind, gimple const gs);
  ActionDataPtr writeKdmUnaryConstructor(gimple const gs);
  ActionDataPtr writeKdmBinaryOperation(KdmKind const & kind, gimple const gs);
  ActionDataPtr writeKdmBinaryOperation(KdmKind const & kind, tree const lhs, tree const rhs1, tree const rhs2);
  ActionDataPtr writeKdmArraySelect(gimple const gs);
  ActionDataPtr writeKdmArraySelect(tree const lhs, tree const rhs, location_t const loc, bool writeblockUnit);
  //ActionDataPtr writeKdmArraySelect(tree const lhs, tree const rhs, location_t const loc, bool writeblockUnit, long & tmpId);
  ActionDataPtr writeKdmArrayReplace(gimple const gs);

  /**
   * D.1716 = this->m_bar;
   */
  ActionDataPtr writeKdmMemberSelect(gimple const gs);
  ActionDataPtr writeKdmMemberSelect(tree const lhs, tree const rhs, location_t const loc, bool writeblockUnit);
  ActionDataPtr writeKdmMemberSelect(long const writesId, long const readsId, long const invokesId);

  /**
   * sin.sin_family = 2;
   */
  ActionDataPtr writeKdmMemberSelectParam(tree const compRef, gimple const gs);
  ActionDataPtr writeKdmMemberSelectParam(tree const compRef, location_t const loc);

  ActionDataPtr writeKdmMemberReplace(gimple const gs);
  ActionDataPtr writeKdmPtr(gimple const gs);
  ActionDataPtr writeKdmPtr(long const lhsId, long const rhsId);
  ActionDataPtr writeKdmPtrParam(tree const addrExpr, gimple const gs);
  ActionDataPtr writeKdmPtrParam(tree const addrExpr, location_t const loc);
  ActionDataPtr writeKdmPtrReplace(gimple const gs);


  long writeKdmStorableUnit(long const typeId, expanded_location const & xloc);
  long writeKdmStorableUnit(long const typeId, location_t loc);

  long getReferenceId(tree const ast);
  ActionDataPtr getRhsReferenceId(tree const rhs);
//  ActionDataPtr getRhsReferenceId(ActionDataPtr mainFlow, tree const rhs);

  tree resolveCall(tree const tree);


  ActionDataPtr updateFlow(ActionDataPtr mainFlow, ActionDataPtr update);
  ActionDataPtr updateActionFlow(ActionDataPtr actionFlow,long const actionId);

  void configureDataAndFlow(ActionDataPtr actionData, ActionDataPtr op0Data, ActionDataPtr op1Data);


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
  ActionDataPtr mLastLabelData;
  long mRegisterVariableIndex;

  ActionDataPtr mLastData;
  bool mHasLastFlow;

  ActionDataPtr mFunctionEntryData;

};

} // namespace kdmtriplewriter

} // namespace gcckdm

#endif /* GCCKDM_KDMTRIPLEWRITER_GIMPLEKDMTRIPLEWRITER_HH_ */
