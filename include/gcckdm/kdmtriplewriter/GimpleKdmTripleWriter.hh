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
  GimpleKdmTripleWriter(KdmTripleWriter & kdmTripleWriter);
  virtual ~GimpleKdmTripleWriter();

  void processGimpleSequence(tree const parent, gimple_seq const gs);
  void processGimpleStatement(tree const parent, gimple const gs);

private:
  typedef std::tr1::unordered_map<expanded_location, long, ExpanedLocationHash, ExpandedLocationEqual> LocationMap;

  long getBlockReferenceId(tree const parent, location_t const loc);
  void processGimpleBindStatement(tree const parent, gimple const gs);
  long processGimpleAssignStatement(tree const parent, gimple const gs);
  long processGimpleReturnStatement(tree const parent, gimple const gs);
  long processGimpleConditionalStatement(tree const parent, gimple const gs);
  long processGimpleLabelStatement(tree const parent, gimple const gs);
  long processGimpleCallStatement(tree const parent, gimple const gs);
  long processGimpleGotoStatement(tree const parent, gimple const gs);

  void processGimpleUnaryAssignStatement(long const actionId, gimple const gs);
  void processGimpleBinaryAssignStatement(long const actionId, gimple const gs);
  void processGimpleTernaryAssignStatement(long const actionId, gimple const gs);

  void processRhsAstNode(long const actionId, tree ast);

  long writeKdmNopForLabel(tree const label);
  long writeKdmActionRelation(KdmType const & type, long const fromId, long const toId);

  void writeKdmUnaryRelationships(long const actionId, long lhsId, long rhsId);
  void writeKdmBinaryRelationships(long const actionId, long lhsId, long rhs1Id, long rhs2Id);

  void writeKdmUnaryOperation(long const actionId, KdmKind const & kind, gimple const gs);
  void writeKdmBinaryOperation(long const actionId, KdmKind const & kind, gimple const gs);
  void writeKdmArraySelect(long const actionId, gimple const gs);
  void writeKdmPtr(long const actionId, gimple const gs);
  void writeKdmPtrReplace(long const actionId, gimple const gs);

  long writeKdmStorableUnit(long const typeId, expanded_location const & xloc);

  long getRhsReferenceId(long actionId, tree const rhs);

  tree resolveCall(tree const tree);

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
};

} // namespace kdmtriplewriter

} // namespace gcckdm

#endif /* GCCKDM_KDMTRIPLEWRITER_GIMPLEKDMTRIPLEWRITER_HH_ */
