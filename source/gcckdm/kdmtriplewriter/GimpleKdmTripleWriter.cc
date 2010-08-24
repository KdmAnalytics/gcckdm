//
// Copyright (c) 2010 KDM Analytics, Inc. All rights reserved.
// Date: Jul 13, 2010
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
#include <boost/format.hpp>
#include <boost/current_function.hpp>
#include <boost/assign/list_of.hpp> // for 'map_list_of()'
#include "gcckdm/kdmtriplewriter/GimpleKdmTripleWriter.hh"
#include "gcckdm/kdmtriplewriter/KdmTripleWriter.hh"
#include "gcckdm/KdmKind.hh"
#include "gcckdm/kdmtriplewriter/Exception.hh"

namespace
{

typedef std::map<tree_code, gcckdm::KdmKind> BinaryOperationKindMap;

//Convienent Map from gcc expressions to their KDM equivalents
BinaryOperationKindMap treeCodeToKind =
    boost::assign::map_list_of(PLUS_EXPR, gcckdm::KdmKind::Add())
                              (POINTER_PLUS_EXPR, gcckdm::KdmKind::Add())
                              (MINUS_EXPR, gcckdm::KdmKind::Subtract())
                              (MULT_EXPR, gcckdm::KdmKind::Multiply())
                              (RDIV_EXPR, gcckdm::KdmKind::Divide())
                              (GT_EXPR, gcckdm::KdmKind::GreaterThan())
                              (GE_EXPR, gcckdm::KdmKind::GreaterThanOrEqual())
                              (LE_EXPR, gcckdm::KdmKind::LessThanOrEqual())
                              (LT_EXPR, gcckdm::KdmKind::LessThan())
                              (EQ_EXPR, gcckdm::KdmKind::Equals())
                              (NE_EXPR, gcckdm::KdmKind::NotEqual())
                              (TRUTH_AND_EXPR, gcckdm::KdmKind::And())
                              (TRUNC_MOD_EXPR, gcckdm::KdmKind::Remainder())
                              (TRUTH_OR_EXPR, gcckdm::KdmKind::Or())
                              (TRUTH_XOR_EXPR, gcckdm::KdmKind::Xor())
                              (LSHIFT_EXPR, gcckdm::KdmKind::LeftShift())
                              (RSHIFT_EXPR, gcckdm::KdmKind::RightShift())
                              (BIT_XOR_EXPR, gcckdm::KdmKind::BitXor())
                              (BIT_AND_EXPR, gcckdm::KdmKind::BitAnd())
                              (BIT_IOR_EXPR, gcckdm::KdmKind::BitOr())
                              ;

bool isValueNode(tree const node)
{
  return TREE_CODE(node) == INTEGER_CST || TREE_CODE(node) == REAL_CST || TREE_CODE(node) == STRING_CST;
}

std::string getUnaryRhsString(gimple const gs)
{
  std::string rhsString;

  enum tree_code gimpleRhsCode = gimple_assign_rhs_code(gs);
  tree lhs = gimple_assign_lhs(gs);
  tree rhs = gimple_assign_rhs1(gs);

  switch (gimpleRhsCode)
  {
    case VIEW_CONVERT_EXPR:
      //Fall Through
    case ASSERT_EXPR:
    {
      rhsString += gcckdm::getAstNodeName(rhs);
      break;
    }
    case FIXED_CONVERT_EXPR:
      //Fall Through
    case ADDR_SPACE_CONVERT_EXPR:
      //Fall Through
    case FIX_TRUNC_EXPR:
      //Fall Through
    case FLOAT_EXPR:
      //Fall Through
    case NOP_EXPR:
      //Fall Through
    case CONVERT_EXPR:
    {
      rhsString += "(" + gcckdm::getAstNodeName(TREE_TYPE(lhs)) + ") ";
      if (op_prio(rhs) < op_code_prio(gimpleRhsCode))
      {
        rhsString += "(" + gcckdm::getAstNodeName(rhs) + ")";
      }
      else
      {
        rhsString += gcckdm::getAstNodeName(rhs);
      }
      break;
    }
    case PAREN_EXPR:
    {
      rhsString += "((" + gcckdm::getAstNodeName(rhs) + "))";
      break;
    }
    case ABS_EXPR:
    {
      rhsString += "ABS_EXPR <" + gcckdm::getAstNodeName(rhs) + ">";
      break;
    }
    default:
    {
      if (TREE_CODE_CLASS(gimpleRhsCode) == tcc_declaration || TREE_CODE_CLASS(gimpleRhsCode) == tcc_constant || TREE_CODE_CLASS(gimpleRhsCode) == tcc_reference
          || gimpleRhsCode == SSA_NAME || gimpleRhsCode == ADDR_EXPR || gimpleRhsCode == CONSTRUCTOR)
      {
        rhsString += gcckdm::getAstNodeName(rhs);
        break;
      }
      else if (gimpleRhsCode == BIT_NOT_EXPR)
      {
        rhsString += '~';
      }
      else if (gimpleRhsCode == TRUTH_NOT_EXPR)
      {
        rhsString += '!';
      }
      else if (gimpleRhsCode == NEGATE_EXPR)
      {
        rhsString += "-";
      }
      else
      {
        rhsString += "[" + std::string(tree_code_name[gimpleRhsCode]) + "]";
      }

      if (op_prio(rhs) < op_code_prio(gimpleRhsCode))
      {
        rhsString += "(" + gcckdm::getAstNodeName(rhs) + ")";
      }
      else
      {
        rhsString += gcckdm::getAstNodeName(rhs);
      }
      break;
    }
  }
  return rhsString;
}

std::string getBinaryRhsString(gimple const gs)
{
  std::string rhsString("");
  enum tree_code code = gimple_assign_rhs_code(gs);
  switch (code)
  {
    case COMPLEX_EXPR:
      //Fall Through
    case MIN_EXPR:
      //Fall Through
    case MAX_EXPR:
      //Fall Through
    case VEC_WIDEN_MULT_HI_EXPR:
      //Fall Through
    case VEC_WIDEN_MULT_LO_EXPR:
      //Fall Through
    case VEC_PACK_TRUNC_EXPR:
      //Fall Through
    case VEC_PACK_SAT_EXPR:
      //Fall Through
    case VEC_PACK_FIX_TRUNC_EXPR:
      //Fall Through
    case VEC_EXTRACT_EVEN_EXPR:
      //Fall Through
    case VEC_EXTRACT_ODD_EXPR:
      //Fall Through
    case VEC_INTERLEAVE_HIGH_EXPR:
      //Fall Through
    case VEC_INTERLEAVE_LOW_EXPR:
    {
      rhsString += tree_code_name[static_cast<int> (code)];
      std::transform(rhsString.begin(), rhsString.end(), rhsString.begin(), toupper);
      rhsString += " <" + gcckdm::getAstNodeName(gimple_assign_rhs1(gs)) + ", " + gcckdm::getAstNodeName(gimple_assign_rhs2(gs)) + ">";
      break;
    }
    default:
    {
      if (op_prio(gimple_assign_rhs1(gs)) <= op_code_prio(code))
      {
        rhsString += "(" + gcckdm::getAstNodeName(gimple_assign_rhs1(gs)) + ")";
      }
      else
      {
        rhsString += gcckdm::getAstNodeName(gimple_assign_rhs1(gs)) + " " + std::string(op_symbol_code(gimple_assign_rhs_code(gs))) + " ";
      }
      if (op_prio(gimple_assign_rhs2(gs)) <= op_code_prio(code))
      {
        rhsString += "(" + gcckdm::getAstNodeName(gimple_assign_rhs2(gs)) + ")";
      }
      else
      {
        rhsString += gcckdm::getAstNodeName(gimple_assign_rhs2(gs));
      }
    }
  }
  return rhsString;

}

std::string getTernaryRhsString(gimple const gs)
{
  return "<TODO: ternary not implemented>";
}

} // namespace

namespace gcckdm
{

namespace kdmtriplewriter
{

GimpleKdmTripleWriter::GimpleKdmTripleWriter(KdmTripleWriter & tripleWriter) :
      mKdmWriter(tripleWriter),
      mLabelFlag(false),
      mRegisterVariableIndex(0),
      mHasLastFlow(false)
{
}

GimpleKdmTripleWriter::~GimpleKdmTripleWriter()
{

}

void GimpleKdmTripleWriter::processAstFunctionDeclarationNode(tree const functionDeclNode)
{
  if (TREE_CODE(functionDeclNode) != FUNCTION_DECL)
  {
    BOOST_THROW_EXCEPTION(InvalidParameterException("'functionDeclNode' was not of type FUNCTION_DECL"));
  }

  if (gimple_has_body_p(functionDeclNode))
  {
    // Clear the block map to ensure we do not reuse blocks in situations where line numbers
    // are duplicated (template functions, for example).
    mBlockUnitMap.clear();

    mCurrentFunctionDeclarationNode = functionDeclNode;
    mCurrentCallableUnitId = getReferenceId(mCurrentFunctionDeclarationNode);
    //
    mHasLastFlow = false;
    mFunctionEntryFlow.reset();

    mKdmWriter.writeComment("================PROCESS BODY START " + gcckdm::getAstNodeName(mCurrentFunctionDeclarationNode) + "==========================");
    gimple_seq seq = gimple_body(mCurrentFunctionDeclarationNode);
    processGimpleSequence(seq);
    mKdmWriter.writeComment("================PROCESS BODY STOP " + gcckdm::getAstNodeName(mCurrentFunctionDeclarationNode) + "==========================");
  }
}



tree GimpleKdmTripleWriter::resolveCall(tree const node)
{
  tree op0 = node;
  tree_code code = TREE_CODE (op0);
  switch (code)
  {
    case VAR_DECL:
    case PARM_DECL:
    case FUNCTION_DECL:
    {
      break;
    }
    case INDIRECT_REF:
    case NOP_EXPR:
    case ADDR_EXPR:
    {
      op0 = TREE_OPERAND(op0, 0);
      resolveCall(op0);
      break;
    }
    default:
    {
      std::string msg(boost::str(boost::format("GIMPLE call statement (%1%) in %2%") % std::string(tree_code_name[TREE_CODE(op0)]) % BOOST_CURRENT_FUNCTION));
      mKdmWriter.writeUnsupportedComment(msg);
    }
    break;
  }
  return op0;
}


long GimpleKdmTripleWriter::getReferenceId(tree const ast)
{
  long retVal;
  retVal = mKdmWriter.getReferenceId(ast);
  return retVal;
}

//GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::getRhsReferenceId(FlowPtr mainFlow, tree const rhs)
//{
//  FlowPtr flow;
//  bool updateFlowFlag = false;
//
//  //Value types we handle special have to add them to languageUnit
//  if (isValueNode(rhs))
//  {
//    mKdmWriter.processAstNode(rhs);
//    flow = FlowPtr(new Flow(getReferenceId(rhs), false));
//    flow->valid = false;
//  }
//  else
//  {
//    if (TREE_CODE(rhs) == CONSTRUCTOR)
//    {
//      flow = FlowPtr(new Flow(getReferenceId(TREE_TYPE(rhs))));
//      updateFlowFlag = true;
//    }
//    else if (TREE_CODE(rhs) == ADDR_EXPR)
//    {
//      flow = writeKdmPtrParam(rhs, gcckdm::locationOf(rhs));
//      updateFlowFlag = true;
//    }
//    else if (TREE_CODE(rhs) == ARRAY_REF)
//    {
//      flow = writeKdmArraySelect(NULL_TREE, rhs, gcckdm::locationOf(rhs), true);
//      updateFlowFlag = true;
//    }
//    else if (TREE_CODE(rhs) == COMPONENT_REF)
//    {
//      flow = writeKdmMemberSelect(NULL_TREE, rhs, gcckdm::locationOf(rhs), true);
//      updateFlowFlag = true;
//    }
//    else
//    {
//      flow = FlowPtr(new Flow(getReferenceId(rhs)));
//      flow->valid = false;
//    }
//
//    if (updateFlowFlag)
//    {
//        writeKdmActionRelation(KdmType::Flow(), mainFlow->end, flow->start);
//    }
//  }
//  return flow;
//}

GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::getRhsReferenceId(tree const rhs)
{
  FlowPtr flow;

  //Value types we handle special have to add them to languageUnit
  if (isValueNode(rhs))
  {
    flow = FlowPtr(new Flow(getReferenceId(rhs)));
    flow->valid = false;
    mKdmWriter.processAstNode(rhs);
  }
  else if (TREE_CODE(rhs) == CONSTRUCTOR)
  {
    flow = FlowPtr(new Flow(getReferenceId(TREE_TYPE(rhs))));
  }
  else if (TREE_CODE(rhs) == ADDR_EXPR)
  {
    flow = writeKdmPtrParam(rhs, gcckdm::locationOf(rhs));
  }
  else if (TREE_CODE(rhs) == ARRAY_REF)
  {
    flow = writeKdmArraySelect(NULL_TREE, rhs, gcckdm::locationOf(rhs), true);
  }
  else if (TREE_CODE(rhs) == COMPONENT_REF)
  {
    flow = writeKdmMemberSelect(NULL_TREE, rhs, gcckdm::locationOf(rhs), true);
  }
  else
  {
    flow = FlowPtr(new Flow(getReferenceId(rhs)));
    flow->valid = false;
  }
  return flow;
}



void GimpleKdmTripleWriter::processGimpleSequence(gimple_seq const seq)
{
  mKdmWriter.writeComment("================GIMPLE START SEQUENCE " + gcckdm::getAstNodeName(mCurrentFunctionDeclarationNode) + "==========================");
  for (gimple_stmt_iterator i = gsi_start(seq); !gsi_end_p(i); gsi_next(&i))
  {
    gimple gs = gsi_stmt(i);
    processGimpleStatement(gs);
  }
  mKdmWriter.writeComment("================GIMPLE END SEQUENCE " + gcckdm::getAstNodeName(mCurrentFunctionDeclarationNode) + "==========================");
}

void GimpleKdmTripleWriter::processGimpleStatement(gimple const gs)
{
  FlowPtr flow;
  if (gs)
  {
    switch (gimple_code(gs))
    {
      case GIMPLE_ASSIGN:
      {
        flow = processGimpleAssignStatement(gs);
        break;
      }
      case GIMPLE_BIND:
      {
        processGimpleBindStatement(gs);
        break;
      }
      case GIMPLE_CALL:
      {
        flow = processGimpleCallStatement(gs);
        break;
      }
      case GIMPLE_COND:
      {
        flow = processGimpleConditionalStatement(gs);
        break;
      }
      case GIMPLE_LABEL:
      {
        processGimpleLabelStatement(gs);
        break;
      }
      case GIMPLE_GOTO:
      {
        processGimpleGotoStatement(gs);
        break;
      }
      case GIMPLE_RETURN:
      {
        flow = processGimpleReturnStatement(gs);
        break;
      }
      case GIMPLE_SWITCH:
      {
        flow = processGimpleSwitchStatement(gs);
        break;
      }
      default:
      {
        std::string msg(boost::str(boost::format("GIMPLE statement (%1%) in %2%") % gimple_code_name[static_cast<int> (gimple_code(gs))] % BOOST_CURRENT_FUNCTION));
        mKdmWriter.writeUnsupportedComment(msg);
        break;
      }
    }

    //If this is the first Action element in the callable unit write an EntryFlow
    if (flow and not mFunctionEntryFlow)
    {
      writeKdmActionRelation(KdmType::EntryFlow(), mCurrentCallableUnitId, flow->start);
      mFunctionEntryFlow = flow;
    }

    //After the first action element we need to hook up the flows
    if (mLastFlow and flow)
    {
      writeKdmActionRelation(KdmType::Flow(), mLastFlow->end, flow->start);
    }

    //the gimple we just processed returned an action remember it
    if (flow)
    {
      mLastFlow = flow;
    }

    // If the last gimple statement we processed was a label or some goto's
    // we have to do a little magic here to get the flows
    // correct... some labels/goto's don't have line numbers so we add
    // the label to the next actionElement after the label
    // mLabelFlag is set in the processGimpleLabelStatement and processGimpleGotoStatement methods
    //
    if (mLabelFlag and flow)
    {
      long blockId = getBlockReferenceId(gimple_location(gs));
      mKdmWriter.writeTripleContains(blockId, mLastLabelFlow->end);
      mLabelFlag = !mLabelFlag;
    }
  }

}

void GimpleKdmTripleWriter::processGimpleBindStatement(gimple const gs)
{
  mKdmWriter.writeComment("================GIMPLE START BIND STATEMENT " + gcckdm::getAstNodeName(mCurrentFunctionDeclarationNode) + "==========================");
  tree var;
  for (var = gimple_bind_vars(gs); var; var = TREE_CHAIN(var))
  {
    if (DECL_EXTERNAL (var))
    {
      mKdmWriter.writeComment("Skipping external variable in bind statement....");
      continue;
    }

    long declId = getReferenceId(var);
    mKdmWriter.processAstNode(var);
    mKdmWriter.writeTripleContains(mCurrentCallableUnitId, declId);
  }
  processGimpleSequence(gimple_bind_body(gs));
  mKdmWriter.writeComment("================GIMPLE END BIND STATEMENT " + gcckdm::getAstNodeName(mCurrentFunctionDeclarationNode) + "==========================");
}

GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::processGimpleAssignStatement(gimple const gs)
{
  FlowPtr flow;
  unsigned numOps(gimple_num_ops(gs));
  if (numOps == 2)
  {
    flow = processGimpleUnaryAssignStatement(gs);
  }
  else if (numOps == 3)
  {
    flow = processGimpleBinaryAssignStatement(gs);
  }
  else if (numOps == 4)
  {
    flow = processGimpleTernaryAssignStatement(gs);
  }
  else
  {
    mKdmWriter.writeComment("GimpleKdmTripleWriter::processGimpleAssignStatement: Unsupported number of operations");
  }

  if (flow)
  {
    //This action element is contained in a block unit
    long blockId = getBlockReferenceId(gimple_location(gs));
    mKdmWriter.writeTripleContains(blockId, flow->start);
  }
  return flow;
}

GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::processGimpleReturnStatement(gimple const gs)
{
  long actionId = mKdmWriter.getNextElementId();
  FlowPtr flow(new Flow(actionId));

  mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionId, KdmKind::Return());
  tree t = gimple_return_retval(gs);
  if (t)
  {
    long id = getReferenceId(t);
    mKdmWriter.processAstNode(t);
    writeKdmActionRelation(KdmType::Reads(), actionId, id);
  }

  //figure out what blockunit this statement belongs
  long blockId = getBlockReferenceId(gimple_location(gs));
  mKdmWriter.writeTripleContains(blockId, actionId);

  return flow;
}

GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::processGimpleLabelStatement(gimple const gs)
{
  tree label = gimple_label_label(gs);
  mLastLabelFlow = writeKdmNopForLabel(label);
  mLabelFlag = true;
  return mLastLabelFlow;
}

GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::writeKdmNopForLabel(tree const label)
{
  long actionId = getReferenceId(label);
  FlowPtr flow(new Flow(actionId));
  mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionId, KdmKind::Nop());
  mKdmWriter.writeTripleName(actionId, gcckdm::getAstNodeName(label));
  return flow;
}


GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::updateActionFlow(FlowPtr actionFlow, long const actionId)
{
  FlowPtr newFlow;

  if (actionFlow)
  {
    actionFlow->end = actionId;
    newFlow = FlowPtr(new Flow(*actionFlow));
  }
  else
  {
    newFlow = FlowPtr(new Flow(actionId));
  }
  return newFlow;
}

GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::updateFlow(FlowPtr mainFlow, FlowPtr flowUpdate)
{
  FlowPtr newFlow;

  if (flowUpdate->valid)
  {
    if (mainFlow)
    {
      writeKdmActionRelation(KdmType::Flow(), mainFlow->end, flowUpdate->start);
      newFlow = FlowPtr(new Flow(mainFlow->start, flowUpdate->end, flowUpdate->valid));
    }
  }
  else
  {
    if (mainFlow)
    {
      newFlow = FlowPtr(new Flow(*mainFlow));
    }
  }
  return newFlow;


//  FlowPtr newFlow = mainFlow;
//
//  //If there isn't a flow to be begin with the flow update is the start
//  if (not newFlow)
//  {
//    newFlow = FlowPtr(new Flow(*flowUpdate));
////    if (flowUpdate->valid)
////    {
////      writeKdmActionRelation(KdmType::Flow(), newFlow->end, flowUpdate->start);
////    }
//  }
//  //otherwise update the end of the flow
//  else
//  {
//    if (flowUpdate->valid)
//    {
//      newFlow->end = flowUpdate->end;
//      writeKdmActionRelation(KdmType::Flow(), mainFlow->end, flowUpdate->start);
//    }
//  }
//  return newFlow;
}

GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::processGimpleConditionalStatement(gimple const gs)
{
  long actionId = mKdmWriter.getNextElementId();

  mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionId, KdmKind::Condition());
  mKdmWriter.writeTripleName(actionId, "if");

  long rhs1Id = getReferenceId(gimple_cond_lhs (gs));
  FlowPtr rhs2Flow = getRhsReferenceId(gimple_cond_rhs(gs));
  FlowPtr actionFlow = updateFlow(FlowPtr(),rhs2Flow);
  long boolId = writeKdmStorableUnit(mKdmWriter.getUserTypeId(KdmType::BooleanType()), gimple_location(gs) );
  mKdmWriter.writeTripleContains(actionId, boolId);

  long condId = mKdmWriter.getNextElementId();
  mKdmWriter.writeTripleKdmType(condId, KdmType::ActionElement());
  mKdmWriter.writeTripleKind(condId, treeCodeToKind.find(gimple_cond_code (gs))->second);
  writeKdmBinaryRelationships(condId, boolId, rhs1Id, rhs2Flow->end);
  long boolBlockId = getBlockReferenceId(gimple_location(gs));
  mKdmWriter.writeTripleContains(boolBlockId, condId);

  writeKdmActionRelation(KdmType::Reads(), actionId, boolId);

  if (gimple_cond_true_label(gs))
  {
    tree trueNode(gimple_cond_true_label(gs));
    long trueFlowId(mKdmWriter.getNextElementId());
    long trueNodeId(getReferenceId(trueNode));

    mKdmWriter.writeTripleKdmType(trueFlowId, KdmType::TrueFlow());
    mKdmWriter.writeTriple(trueFlowId, KdmPredicate::From(), actionId);
    mKdmWriter.writeTriple(trueFlowId, KdmPredicate::To(), trueNodeId);
    mKdmWriter.writeTripleContains(actionId, trueFlowId);
    actionFlow = updateActionFlow(actionFlow, trueFlowId);
  }
  if (gimple_cond_false_label(gs))
  {
    tree falseNode(gimple_cond_false_label(gs));
    long falseFlowId(mKdmWriter.getNextElementId());
    long falseNodeId(getReferenceId(falseNode));
    mKdmWriter.writeTripleKdmType(falseFlowId, KdmType::FalseFlow());
    mKdmWriter.writeTriple(falseFlowId, KdmPredicate::From(), actionId);
    mKdmWriter.writeTriple(falseFlowId, KdmPredicate::To(), falseNodeId);
    mKdmWriter.writeTripleContains(actionId, falseFlowId);
    actionFlow = updateActionFlow(actionFlow, falseFlowId);
  }

  long blockId = getBlockReferenceId(gimple_location(gs));
  mKdmWriter.writeTripleContains(blockId, actionId);

  return actionFlow;
}

GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::processGimpleCallStatement(gimple const gs)
{
  long actionId(mKdmWriter.getNextElementId());
  FlowPtr flow;

  mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());

  tree op0 = gimple_call_fn(gs);
  if (TREE_CODE (op0) == NON_LVALUE_EXPR)
  {
    op0 = TREE_OPERAND (op0, 0);
  }
  tree t(resolveCall(op0));
  long callableId(getReferenceId(t));

  if (TREE_CODE(TREE_TYPE(t)) == POINTER_TYPE)
  {
    //it's a function pointer call
    mKdmWriter.writeTripleKind(actionId, KdmKind::PtrCall());
    writeKdmActionRelation(KdmType::Addresses(), actionId, callableId);
  }
  else
  {
    //regular call
    mKdmWriter.writeTripleKind(actionId, KdmKind::Call());
    long callId = mKdmWriter.getNextElementId();
    mKdmWriter.writeTripleKdmType(callId, KdmType::Calls());
    mKdmWriter.writeTriple(callId, KdmPredicate::From(), actionId);
    mKdmWriter.writeTriple(callId, KdmPredicate::To(), callableId);
    mKdmWriter.writeTripleContains(actionId, callId);
  }

  //Read each parameter
  if (gimple_call_num_args(gs) > 0)
  {
    for (size_t i = 0; i < gimple_call_num_args(gs); i++)
    {
      tree callArg(gimple_call_arg(gs, i));
      FlowPtr paramFlow;
      //parameter to a function is taking the address of something
      if (TREE_CODE(callArg) == ADDR_EXPR)
      {
        paramFlow = writeKdmPtrParam(callArg, gs);
      }
      else if (TREE_CODE(callArg) == COMPONENT_REF)
      {
        paramFlow = writeKdmMemberSelectParam(callArg, gs);
      }
      else
      {
        paramFlow = FlowPtr(new Flow(getReferenceId(callArg)));
      }

      writeKdmActionRelation(KdmType::Reads(), actionId, paramFlow->end);
      if (not flow and paramFlow->valid)
      {
          flow = paramFlow;
      }
      else if (flow)
      {
        flow->end = paramFlow->end;
      }
      else
      {
        //do nothing
      }
    }
  }
  else
  {
    flow = FlowPtr(new Flow(actionId));
  }

  //optional write
  tree lhs = gimple_call_lhs(gs);
  if (lhs)
  {
    long lhsId(getReferenceId(lhs));
    writeKdmActionRelation(KdmType::Writes(), actionId, lhsId);
  }

  long blockId = getBlockReferenceId(gimple_location(gs));
  mKdmWriter.writeTripleContains(blockId, actionId);

  return flow;
}


GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::processGimpleGotoStatement(gimple const gs)
{
  long actionId(mKdmWriter.getNextElementId());
  FlowPtr flow(new Flow(actionId));

  mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionId, KdmKind::Goto());

  tree label(gimple_goto_dest (gs));
  long destId(getReferenceId(label));
  mKdmWriter.writeTriple(actionId, KdmPredicate::From(), actionId);
  mKdmWriter.writeTriple(actionId, KdmPredicate::To(), destId);

  //figure out what blockunit this statement belongs

  if (!gimple_location(gs))
  {
    //this goto doesn't have a location.... use the destination location?
    mKdmWriter.writeComment("FIXME: This GOTO doesn't have a location... what should we use instead");
    mLastLabelFlow = flow;
    mLabelFlag = true;
  }
  else
  {
    long blockId = getBlockReferenceId(gimple_location(gs));
    mKdmWriter.writeTripleContains(blockId, actionId);
  }
  return flow;
}


GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::processGimpleUnaryAssignStatement(gimple const gs)
{
  FlowPtr flow;
  tree rhsNode = gimple_assign_rhs1(gs);
  enum tree_code gimpleRhsCode = gimple_assign_rhs_code(gs);
  switch (gimpleRhsCode)
  {
    case VIEW_CONVERT_EXPR:
      //Fall Through
    case ASSERT_EXPR:
    {
      std::string msg(boost::str(boost::format("GIMPLE assignment statement (%1%) in %2%") % std::string(tree_code_name[gimpleRhsCode])
      % BOOST_CURRENT_FUNCTION));
      mKdmWriter.writeUnsupportedComment(msg);
      break;
    }
    case FIXED_CONVERT_EXPR:
      //Fall Through
    case ADDR_SPACE_CONVERT_EXPR:
      //Fall Through
    case FIX_TRUNC_EXPR:
      //Fall Through
    case FLOAT_EXPR:
      //Fall Through
    case NOP_EXPR:
      //Fall Through
    case CONVERT_EXPR:
    {
      //Casting a pointer
      if (TREE_CODE(rhsNode) == ADDR_EXPR)
      {
        flow = writeKdmPtr(gs);
        break;
      }
      else
      {
        //Simple cast, no pointers or references
        mKdmWriter.writeComment("FIXME: This Assign is really a cast, but we do not support casts");
        flow = writeKdmUnaryOperation(KdmKind::Assign(), gs);
      }
      break;
    }
    case PAREN_EXPR:
       //Fall Through
    case ABS_EXPR:
    {
      std::string msg(boost::str(boost::format("GIMPLE assignment operation (%1%) in %2%") % std::string(tree_code_name[gimpleRhsCode])
      % BOOST_CURRENT_FUNCTION));
      mKdmWriter.writeUnsupportedComment(msg);
      //            rhsString += "ABS_EXPR <" + gcckdm::getAstNodeName(rhs) + ">";
      break;
    }
    default:
    {
      if (TREE_CODE_CLASS(gimpleRhsCode) == tcc_declaration || TREE_CODE_CLASS(gimpleRhsCode) == tcc_constant
          || gimpleRhsCode == SSA_NAME || gimpleRhsCode == CONSTRUCTOR)
      {
        tree lhs = gimple_assign_lhs(gs);
        if (TREE_CODE(lhs) == INDIRECT_REF)
        {
          flow = writeKdmPtrReplace(gs);
        }
        else if (TREE_CODE(lhs) == COMPONENT_REF)
        {
          flow = writeKdmMemberReplace(gs);
        }
        else if (TREE_CODE(lhs) == ARRAY_REF)
        {
          flow = writeKdmArrayReplace(gs);
        }
        else
        {
          flow = writeKdmUnaryOperation(KdmKind::Assign(), gs);
        }
        break;
      }
      else if (TREE_CODE_CLASS(gimpleRhsCode) == tcc_reference)
      {
        if (gimpleRhsCode == ARRAY_REF)
        {
          flow = writeKdmArraySelect(gs);
        }
        else if(gimpleRhsCode == COMPONENT_REF)
        {
          flow = writeKdmMemberSelect(gs);
          break;
        }
        else if (gimpleRhsCode == INDIRECT_REF)
        {
          flow = writeKdmPtr(gs);
        }
        else
        {
          std::string msg(boost::str(boost::format("GIMPLE assignment operation (%1%) in %2% on line (%3%)") % std::string(tree_code_name[gimpleRhsCode])
          % BOOST_CURRENT_FUNCTION % __LINE__));
          mKdmWriter.writeUnsupportedComment(msg);
        }
        break;
      }
      else if (gimpleRhsCode == ADDR_EXPR)
      {
        flow = writeKdmPtr(gs);
        break;
      }
      else if (gimpleRhsCode == NEGATE_EXPR)
      {
        flow = writeKdmUnaryOperation(KdmKind::Negate(), gs);
        break;
      }
      else if (gimpleRhsCode == BIT_NOT_EXPR)
      {
        flow = writeKdmUnaryOperation(KdmKind::BitNot(), gs);
        break;
      }
      else
      {
        std::string msg(boost::str(boost::format("GIMPLE assignment operation (%1%) in %2% on line (%3%)") % std::string(tree_code_name[gimpleRhsCode])  % BOOST_CURRENT_FUNCTION % __LINE__));
        mKdmWriter.writeUnsupportedComment(msg);
        break;
      }
      break;
    }
  }
  return flow;
}




GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::processGimpleBinaryAssignStatement(gimple const gs)
{
  FlowPtr flow;
  enum tree_code code = gimple_assign_rhs_code(gs);
  switch (code)
  {
    case COMPLEX_EXPR:
    case MIN_EXPR:
    case MAX_EXPR:
    case VEC_WIDEN_MULT_HI_EXPR:
    case VEC_WIDEN_MULT_LO_EXPR:
    case VEC_PACK_TRUNC_EXPR:
    case VEC_PACK_SAT_EXPR:
    case VEC_PACK_FIX_TRUNC_EXPR:
    case VEC_EXTRACT_EVEN_EXPR:
    case VEC_EXTRACT_ODD_EXPR:
    case VEC_INTERLEAVE_HIGH_EXPR:
    case VEC_INTERLEAVE_LOW_EXPR:
    {
      //Do Nothing
      break;
    }
    default:
    {
      if (op_prio(gimple_assign_rhs1(gs)) <= op_code_prio(code))
      {
        //Do Nothing
        break;
      }
      else
      {
        switch (gimple_assign_rhs_code(gs))
        {
          case PLUS_EXPR:
          case POINTER_PLUS_EXPR:
          case MINUS_EXPR:
          case MULT_EXPR:
          case RDIV_EXPR:
          case TRUNC_MOD_EXPR:
          case LT_EXPR:
          case LE_EXPR:
          case GT_EXPR:
          case GE_EXPR:
          case EQ_EXPR:
          case NE_EXPR:
          case LSHIFT_EXPR:
          case RSHIFT_EXPR:
          case BIT_XOR_EXPR:
          case BIT_IOR_EXPR:
          case BIT_AND_EXPR:
          {
            flow = writeKdmBinaryOperation(treeCodeToKind.find(gimple_assign_rhs_code(gs))->second, gs);
            break;
          }
          default:
          {
            std::string msg(boost::str(boost::format("GIMPLE binary assignment operation '%1%' (%2%) in %3% on line %4%") % op_symbol_code(gimple_assign_rhs_code(gs)) % std::string(tree_code_name[gimple_assign_rhs_code(gs)]) % BOOST_CURRENT_FUNCTION % __LINE__));
            mKdmWriter.writeUnsupportedComment(msg);
            break;
          }
        }
      }
    }
  }
  return flow;
}

GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::processGimpleTernaryAssignStatement(gimple const gs)
{
  enum tree_code rhs_code = gimple_assign_rhs_code(gs);
  std::string msg(boost::str(boost::format("GIMPLE assignment operation (%1%) in %2%") % std::string(tree_code_name[rhs_code])
  % BOOST_CURRENT_FUNCTION));
  mKdmWriter.writeUnsupportedComment(msg);
  return FlowPtr();
}


GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::processGimpleSwitchStatement(gimple const gs)
{
  long actionId(mKdmWriter.getNextElementId());
  FlowPtr flow(new Flow(actionId));

  mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
  //switch variable
  long indexId = getReferenceId(gimple_switch_index (gs));
  mKdmWriter.writeTripleKind(actionId, KdmKind::Switch());
  writeKdmActionRelation(KdmType::Reads(), actionId, indexId);
  if (gimple_location(gs))
  {
    long blockId = getBlockReferenceId(gimple_location(gs));
    mKdmWriter.writeTripleContains(blockId, actionId);
  }

  for (unsigned i = 0; i < gimple_switch_num_labels (gs); ++i)
  {
    tree caseLabel(gimple_switch_label (gs, i));
    if (caseLabel == NULL_TREE)
    {
      continue;
    }

    if (CASE_LOW (caseLabel) && CASE_HIGH (caseLabel))
    {
      std::string msg(boost::str(boost::format("GIMPLE switch statement CASE_LOW && CASE_HIGH in %2% on line (%3%)") %  BOOST_CURRENT_FUNCTION % __LINE__));
      mKdmWriter.writeUnsupportedComment(msg);
    }
    else if (CASE_LOW(caseLabel))
    {
      tree caseNode(CASE_LABEL (caseLabel));
      long caseNodeId(getReferenceId(caseNode));
      long readsId = writeKdmActionRelation(KdmType::Reads(), actionId, caseNodeId);

      long guardedFlowId(mKdmWriter.getNextElementId());
      mKdmWriter.writeTripleKdmType(guardedFlowId, KdmType::GuardedFlow());
      mKdmWriter.writeTriple(guardedFlowId, KdmPredicate::From(), guardedFlowId);
      mKdmWriter.writeTriple(guardedFlowId, KdmPredicate::To(), readsId);
      mKdmWriter.writeTripleContains(actionId, guardedFlowId);
      flow->end = guardedFlowId;
    }
    else
    {
      //default is the false flow
      tree falseNode(CASE_LABEL (caseLabel));
      long falseFlowId(mKdmWriter.getNextElementId());
      long falseNodeId(getReferenceId(falseNode));
      mKdmWriter.writeTripleKdmType(falseFlowId, KdmType::FalseFlow());
      mKdmWriter.writeTriple(falseFlowId, KdmPredicate::From(), actionId);
      mKdmWriter.writeTriple(falseFlowId, KdmPredicate::To(), falseNodeId);
      mKdmWriter.writeTripleContains(actionId, falseFlowId);
      flow->end = falseFlowId;
    }
  }
  return flow;
}

long GimpleKdmTripleWriter::getBlockReferenceId(location_t const loc)
{
  if (loc == 0)
  {
    BOOST_THROW_EXCEPTION(NullLocationException());
  }

  expanded_location xloc = expand_location(loc);
  LocationMap::iterator i = mBlockUnitMap.find(xloc);
  long blockId;
  if (i == mBlockUnitMap.end())
  {
    blockId = mKdmWriter.getNextElementId();
    mBlockUnitMap.insert(std::make_pair(xloc, blockId));
    mKdmWriter.writeTripleKdmType(blockId, KdmType::BlockUnit());
    mKdmWriter.writeKdmSourceRef(blockId, xloc);
    mKdmWriter.writeTripleContains(mCurrentCallableUnitId, blockId);
  }
  else
  {
    blockId = i->second;
  }
  return blockId;
}


long GimpleKdmTripleWriter::writeKdmActionRelation(KdmType const & type, long const fromId, long const toId)
{
  long arId = mKdmWriter.getNextElementId();
  mKdmWriter.writeTripleKdmType(arId, type);
  mKdmWriter.writeTriple(arId, KdmPredicate::From(), fromId);
  mKdmWriter.writeTriple(arId, KdmPredicate::To(), toId);
  mKdmWriter.writeTripleContains(fromId, arId);
  return arId;
}

void GimpleKdmTripleWriter::writeKdmUnaryRelationships(long const actionId, long const lhsId, long const rhsId)
{
  writeKdmActionRelation(KdmType::Reads(), actionId, rhsId);
  writeKdmActionRelation(KdmType::Writes(), actionId, lhsId);
}


GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::writeKdmUnaryOperation(KdmKind const & kind, gimple const gs)
{
  tree lhs(gimple_assign_lhs(gs));
  tree rhs(gimple_assign_rhs1(gs));
  return writeKdmUnaryOperation(kind, lhs, rhs);
}


GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::writeKdmUnaryOperation(KdmKind const & kind, tree const lhs, tree const rhs)
{
  long actionId = mKdmWriter.getNextElementId();
  long lhsId = getReferenceId(lhs);
  FlowPtr rhsFlow = getRhsReferenceId(rhs);
  FlowPtr actionFlow = updateFlow(FlowPtr(), rhsFlow);
  mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionId, kind);
  writeKdmUnaryRelationships(actionId, lhsId, rhsFlow->end);
  return updateActionFlow(actionFlow, actionId);
}


GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::writeKdmPtrReplace(gimple const gs)
{
  tree lhs = gimple_assign_lhs(gs);
  tree rhs = gimple_assign_rhs1(gs);
  long actionId = mKdmWriter.getNextElementId();
  tree lhsOp0 = TREE_OPERAND (lhs, 0);
  long lhsId = getReferenceId(lhsOp0);
  FlowPtr rhsFlow = getRhsReferenceId(rhs);
  FlowPtr actionFlow = updateFlow(FlowPtr(), rhsFlow);


  mKdmWriter.writeTripleKind(actionId, KdmKind::PtrReplace());
  writeKdmActionRelation(KdmType::Reads(), actionId, rhsFlow->end);
  writeKdmActionRelation(KdmType::Addresses(), actionId, lhsId);

  //Have to determine if there is a bug in the spec here or not
  //where does the write relationship go?
  mKdmWriter.writeComment("FIXME: KDM spec states there should be a writes relationship here.. ");
  //writeKdmActionRelation(KdmType::Writes(), actionId, lhsId);
  return updateActionFlow(actionFlow, actionId);
}

GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::writeKdmBinaryOperation(KdmKind const & kind, gimple const gs)
{
  tree lhs(gimple_assign_lhs(gs));
  tree rhs1(gimple_assign_rhs1(gs));
  tree rhs2(gimple_assign_rhs2(gs));
  return writeKdmBinaryOperation(kind, lhs, rhs1, rhs2);
}

GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::writeKdmBinaryOperation(KdmKind const & kind, tree const lhs, tree const rhs1, tree const rhs2)
{
  long actionId = mKdmWriter.getNextElementId();
  mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionId, kind);

  FlowPtr rhs1Flow = getRhsReferenceId(rhs1);
  FlowPtr actionFlow = updateFlow(FlowPtr(), rhs1Flow);
  FlowPtr rhs2Flow = getRhsReferenceId(rhs2);
  actionFlow = updateFlow(actionFlow, rhs2Flow);

  long lhsId = getReferenceId(lhs);
  writeKdmBinaryRelationships(actionId, lhsId, rhs1Flow->end, rhs2Flow->end);
  return updateActionFlow(actionFlow, actionId);
}


//D.1989 = a[23];
GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::writeKdmArraySelect(gimple const gs)
{
  tree lhs = gimple_assign_lhs(gs);
  tree rhs = gimple_assign_rhs1(gs);
  return writeKdmArraySelect(lhs, rhs, gimple_location(gs), false);
}


GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::writeKdmArraySelect(tree const lhs, tree const rhs, location_t const loc, bool writeBlockUnit)
{
  assert(TREE_CODE(rhs) == ARRAY_REF);

  long actionId(mKdmWriter.getNextElementId());
  expanded_location xloc = expand_location(loc);
  tree op0 = TREE_OPERAND (rhs, 0);
  tree op1 = TREE_OPERAND (rhs, 1);
  FlowPtr op0Flow = getRhsReferenceId(op0);
  FlowPtr actionFlow = updateFlow(FlowPtr(), op0Flow);
  FlowPtr op1Flow = getRhsReferenceId(op1);
  actionFlow = updateFlow(actionFlow, op1Flow);

  long lhsId = (lhs == NULL_TREE) ? writeKdmStorableUnit(getReferenceId(TREE_TYPE(op0)),xloc) : getReferenceId(lhs);
  mKdmWriter.writeTripleContains(actionId, lhsId);


  mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionId, KdmKind::ArraySelect());
  writeKdmActionRelation(KdmType::Reads(), actionId, op0Flow->end);
  writeKdmActionRelation(KdmType::Addresses(), actionId, op0Flow->end);
  writeKdmActionRelation(KdmType::Writes(), actionId, lhsId);

  if (writeBlockUnit)
  {
    const long blockUnitId(getBlockReferenceId(loc));
    mKdmWriter.writeTripleContains(blockUnitId, lhsId);
  }
  return updateActionFlow(actionFlow, actionId);
}


// foo[0] = 1
GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::writeKdmArrayReplace(gimple const gs)
{
  tree lhs = gimple_assign_lhs(gs);
  tree rhs = gimple_assign_rhs1(gs);
  tree op0 = TREE_OPERAND (lhs, 0);
  tree op1 = TREE_OPERAND (lhs, 1);
  FlowPtr rhsFlow(getRhsReferenceId(rhs));
  FlowPtr actionFlow = updateFlow(FlowPtr(), rhsFlow);
  long op0Id = getReferenceId(op0);
  long op1Id = getReferenceId(op1);
  long actionId = mKdmWriter.getNextElementId();


  mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionId, KdmKind::ArrayReplace());
  writeKdmActionRelation(KdmType::Addresses(), actionId, op0Id); //data element
  writeKdmActionRelation(KdmType::Reads(), actionId, op1Id); // index
  writeKdmActionRelation(KdmType::Reads(), actionId, rhsFlow->end); //new value

  //Have to determine if there is a bug in the spec here or not
  //where does the write relationship go?
  mKdmWriter.writeComment("FIXME: KDM spec states there should be a writes relationship here..");
  return updateActionFlow(actionFlow, actionId);
}


GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::writeKdmMemberSelect(tree const lhs, tree const rhs, location_t const loc, bool writeBlockUnit)
{
  assert(TREE_CODE(rhs) == COMPONENT_REF);

  expanded_location xloc = expand_location(loc);
  tree op0 = TREE_OPERAND (rhs, 0);
  tree op1 = TREE_OPERAND (rhs, 1);
  FlowPtr actionFlow;
  long lhsId;

  if (TREE_CODE(op0) == INDIRECT_REF)
  {
    //we have to create a temp variable to hold the Ptr result
    tree indirectRef = TREE_OPERAND (op0, 0);
    long refId = getReferenceId(indirectRef);
    lhsId = writeKdmStorableUnit(getReferenceId(TREE_TYPE(indirectRef)),loc);




//    //Resolve the indirect reference put result in temp
//    mKdmWriter.writeTripleKdmType(ptrActionId, KdmType::ActionElement());
//    mKdmWriter.writeTripleKind(ptrActionId, KdmKind::Ptr());
//    writeKdmActionRelation(KdmType::Addresses(), ptrActionId, refId);
//    writeKdmActionRelation(KdmType::Writes(), ptrActionId, storableId);
    FlowPtr ptrFlow = writeKdmPtr(lhsId, refId);
    actionFlow = updateFlow(FlowPtr(), ptrFlow);
    mKdmWriter.writeTripleContains(ptrFlow->end, lhsId);

    //perform memberselect using temp
    FlowPtr op1Flow = getRhsReferenceId(op1);
    actionFlow = updateFlow(actionFlow, op1Flow);
//    mKdmWriter.writeTripleKind(actionElementId, KdmKind::MemberSelect());
//    writeKdmActionRelation(KdmType::Reads(), actionElementId, op1Id);
//    writeKdmActionRelation(KdmType::Invokes(), actionElementId, ptrActionId);
//    writeKdmActionRelation(KdmType::Writes(), actionElementId, lhsId);

    FlowPtr memberSelectFlow = writeKdmMemberSelect(lhsId, op1Flow->end, ptrFlow->end);
    actionFlow = updateFlow(actionFlow, memberSelectFlow);

    if (writeBlockUnit)
    {
      const long blockUnitId(getBlockReferenceId(loc));
      mKdmWriter.writeTripleContains(blockUnitId, lhsId);
    }

  }
  else
  {
    if (not lhs)
    {
      lhsId = writeKdmStorableUnit(getReferenceId(TREE_TYPE(rhs)),loc);
    }

    FlowPtr op0Flow(getRhsReferenceId(op0));
    actionFlow = updateFlow(FlowPtr(), op0Flow);
    FlowPtr op1Flow(getRhsReferenceId(op1));
    actionFlow = updateFlow(actionFlow, op1Flow);

//    writeKdmActionRelation(KdmType::Reads(), actionElementId, op1Id);
//    writeKdmActionRelation(KdmType::Invokes(), actionElementId, op0Id);
//    writeKdmActionRelation(KdmType::Writes(), actionElementId, lhsId);
    FlowPtr memberFlow = writeKdmMemberSelect(lhsId, op1Flow->end, op0Flow->end);
    actionFlow = updateActionFlow(actionFlow, memberFlow->end);

  }
  return actionFlow;
}

GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::writeKdmMemberSelect(long const writesId, long const readsId, long const invokesId)
{
  long actionId = mKdmWriter.getNextElementId();
  FlowPtr flow(new Flow(actionId));
  mKdmWriter.writeTripleKind(actionId, KdmKind::MemberSelect());
  writeKdmActionRelation(KdmType::Reads(), actionId, readsId);
  writeKdmActionRelation(KdmType::Invokes(), actionId, invokesId);
  writeKdmActionRelation(KdmType::Writes(), actionId, writesId);
  return flow;
}

/** Write to LHS
 * Addresses object
 * Reads member
 *
 */
//D.1716 = this->m_bar;
//D.4427 = hp->h_length;
GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::writeKdmMemberSelect(gimple const gs)
{
  tree lhs = gimple_assign_lhs(gs);
  tree rhs = gimple_assign_rhs1(gs);
  tree op0 = TREE_OPERAND (rhs, 0);
  tree op1 = TREE_OPERAND (rhs, 1);
  long lhsId = getReferenceId(lhs);

  FlowPtr actionFlow;

  if (TREE_CODE(op0) == INDIRECT_REF)
  {
    tree indirectRef = TREE_OPERAND (op0, 0);
    long refId = getReferenceId(indirectRef);
    FlowPtr op1Flow = getRhsReferenceId(op1);
    actionFlow = updateFlow(FlowPtr(), op1Flow);

    //we have to create a temp variable to hold the Ptr result
    long storableId = writeKdmStorableUnit(getReferenceId(TREE_TYPE(indirectRef)),expand_location(gimple_location(gs)));

    //Resolve the indirect reference put result in temp
    long ptrActionId = mKdmWriter.getNextElementId();
    mKdmWriter.writeTripleKdmType(ptrActionId, KdmType::ActionElement());
    mKdmWriter.writeTripleKind(ptrActionId, KdmKind::Ptr());
    writeKdmActionRelation(KdmType::Addresses(), ptrActionId, refId);
    writeKdmActionRelation(KdmType::Writes(), ptrActionId, storableId);
    mKdmWriter.writeTripleContains(ptrActionId, storableId);


    //perform memberselect using temp
    long actionId = mKdmWriter.getNextElementId();
    mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
    mKdmWriter.writeTripleKind(actionId, KdmKind::MemberSelect());
    writeKdmActionRelation(KdmType::Reads(), actionId, op1Flow->end);
    writeKdmActionRelation(KdmType::Invokes(), actionId, ptrActionId);
    writeKdmActionRelation(KdmType::Writes(), actionId, lhsId);
    actionFlow = updateActionFlow(actionFlow, actionId);
  }
  else
  {
    long actionId = mKdmWriter.getNextElementId();
    mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
    mKdmWriter.writeTripleKind(actionId, KdmKind::MemberSelect());
    FlowPtr op0Flow = getRhsReferenceId(op0);
    actionFlow = updateFlow(FlowPtr(), op0Flow);
    FlowPtr op1Flow = getRhsReferenceId(op1);
    actionFlow = updateFlow(FlowPtr(), op1Flow);

    writeKdmActionRelation(KdmType::Reads(), actionId, op1Flow->end);
    writeKdmActionRelation(KdmType::Invokes(), actionId, op0Flow->end);
    writeKdmActionRelation(KdmType::Writes(), actionId, lhsId);
    actionFlow = updateActionFlow(actionFlow, actionId);
  }
  return actionFlow;
}

GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::writeKdmMemberSelectParam(tree const compRef, gimple const gs)
{
  return writeKdmMemberSelectParam(compRef, gimple_location(gs));
}

GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::writeKdmMemberSelectParam(tree const compRef, location_t const loc)
{
  const long blockUnitId(getBlockReferenceId(loc));
  tree op0 = TREE_OPERAND (compRef, 0);
  tree op1 = TREE_OPERAND (compRef, 1);

  long actionId = mKdmWriter.getNextElementId();
  FlowPtr actionFlow;

  mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionId, KdmKind::MemberSelect());

  //we have to create a temp variable
  long storableId = writeKdmStorableUnit(getReferenceId(TREE_TYPE(op0)), loc);
  mKdmWriter.writeTripleContains(blockUnitId, storableId);

  FlowPtr op0Flow = getRhsReferenceId(op0);
  actionFlow = updateFlow(FlowPtr(), op0Flow);
  FlowPtr op1Flow = getRhsReferenceId(op1);
  actionFlow = updateFlow(FlowPtr(), op1Flow);

  writeKdmActionRelation(KdmType::Reads(), actionId, op1Flow->end);
  writeKdmActionRelation(KdmType::Invokes(), actionId, op0Flow->end);
  writeKdmActionRelation(KdmType::Writes(), actionId, storableId);
  return updateActionFlow(actionFlow, actionId);
}


//Example: sin.sin_family = 2;
GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::writeKdmMemberReplace(gimple const gs)
{
  tree lhs = gimple_assign_lhs(gs);
  tree rhs = gimple_assign_rhs1(gs);
  tree lhsOp0 = TREE_OPERAND (lhs, 0);
  long lhsId = getReferenceId(lhsOp0);
  FlowPtr rhsFlow = getRhsReferenceId(rhs);
  FlowPtr actionFlow = updateFlow(FlowPtr(), rhsFlow);
  long actionId = mKdmWriter.getNextElementId();

  mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionId, KdmKind::MemberReplace());
  writeKdmActionRelation(KdmType::Reads(), actionId, rhsFlow->end);
  writeKdmActionRelation(KdmType::Invokes(), actionId, lhsId);
  writeKdmActionRelation(KdmType::Writes(), actionId, lhsId);
  return updateActionFlow(actionFlow, actionId);
}


//ptr = &a[0];
GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::writeKdmPtr(gimple const gs)
{
  FlowPtr actionFlow;
  tree lhs = gimple_assign_lhs(gs);
  //this should be an addr_expr
  tree rhs = gimple_assign_rhs1(gs);
  tree op0 = TREE_OPERAND (rhs, 0);

  if (TREE_CODE(op0) == ARRAY_REF)
  {
    //Write Array Select Action Element into a temporary
    actionFlow = writeKdmArraySelect(NULL_TREE, op0, gimple_location(gs), true);

//    tree lhs = gimple_assign_lhs(gs);
//    long lhsId = getReferenceId(lhs);
//    writeKdmActionRelation(KdmType::Writes(), ptrActionId, lhsId);
    FlowPtr ptrFlow(writeKdmPtr(getReferenceId(lhs), actionFlow->start));
    actionFlow = updateActionFlow(actionFlow, ptrFlow->end);
  }
  else
  {
    FlowPtr rhsFlow = getRhsReferenceId(op0);
    actionFlow = updateFlow(FlowPtr(), rhsFlow);
    FlowPtr ptrFlow = writeKdmPtr(getReferenceId(lhs),rhsFlow->end);
    actionFlow = updateActionFlow(actionFlow, ptrFlow->end);
  }
  return actionFlow;
}

GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::writeKdmPtr(long const writesId, long const addressesId)
{
  long actionId = mKdmWriter.getNextElementId();
  FlowPtr actionFlow(new Flow(actionId));
  mKdmWriter.writeTripleKind(actionId, KdmKind::Ptr());
  writeKdmActionRelation(KdmType::Addresses(), actionId, addressesId);
  writeKdmActionRelation(KdmType::Writes(), actionId, writesId);
  return actionFlow;
}



GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::writeKdmPtrParam(tree const addrExpr, gimple const gs)
{
  return writeKdmPtrParam(addrExpr, gimple_location(gs));
}

GimpleKdmTripleWriter::FlowPtr GimpleKdmTripleWriter::writeKdmPtrParam(tree const addrExpr, location_t const loc)
{
  //All of these elements are contained in the same block unit
  const long blockUnitId(getBlockReferenceId(loc));
  tree op0 = TREE_OPERAND (addrExpr, 0);
  FlowPtr actionFlow;

  if (TREE_CODE(op0) == ARRAY_REF)
  {
    tree refOp0 = TREE_OPERAND (op0, 0);
    FlowPtr selectFlow = writeKdmArraySelect(NULL_TREE, op0, loc, true);
    actionFlow = selectFlow;

    //we have to create a second temp variable... aka temp2
    long storable2Id = writeKdmStorableUnit(getReferenceId(TREE_TYPE(refOp0)),gcckdm::locationOf(addrExpr));
    mKdmWriter.writeTripleContains(blockUnitId, storable2Id);

    //perform address on temp1 assign to temp2    temp2 = &rhs
    FlowPtr ptrFlow(writeKdmPtr(storable2Id, selectFlow->end));
    mKdmWriter.writeTripleContains(blockUnitId, ptrFlow->end);
    actionFlow = updateActionFlow(actionFlow, ptrFlow->end);

  }
  else if (TREE_CODE(op0) == COMPONENT_REF)
  {
    actionFlow = writeKdmMemberSelectParam(op0, loc);
  }
  else
  {
    //we have to create a temp variable... aka temp1
    long storableId = writeKdmStorableUnit(getReferenceId(TREE_TYPE(TREE_OPERAND (addrExpr, 0))),gcckdm::locationOf(addrExpr));
    mKdmWriter.writeTripleContains(blockUnitId, storableId);

    //Address.... temp1 = &rhs
    FlowPtr rhsFlow = getRhsReferenceId( op0);
    actionFlow = updateFlow(FlowPtr(), rhsFlow);
    FlowPtr ptrFlow = writeKdmPtr(storableId, rhsFlow->end);
    mKdmWriter.writeTripleContains(blockUnitId, rhsFlow->end);
    actionFlow = updateActionFlow(actionFlow, ptrFlow->end);

  }
  return actionFlow;
}

long GimpleKdmTripleWriter::writeKdmStorableUnit(long const typeId, location_t const loc)
{
  return writeKdmStorableUnit(typeId, expand_location(loc));
}

long GimpleKdmTripleWriter::writeKdmStorableUnit(long const typeId, expanded_location const & xloc)
{
  long unitId = mKdmWriter.getNextElementId();
  mKdmWriter.writeTripleKdmType(unitId, KdmType::StorableUnit());
  mKdmWriter.writeTripleName(unitId, "M." + boost::lexical_cast<std::string>(++mRegisterVariableIndex));
  mKdmWriter.writeTripleKind(unitId, KdmKind::Register());
  mKdmWriter.writeTriple(unitId, KdmPredicate::Type(), typeId);
  mKdmWriter.writeKdmSourceRef(unitId, xloc);
  return unitId;
}


void GimpleKdmTripleWriter::writeKdmBinaryRelationships(long const actionId, long const lhsId, long const rhs1Id, long const rhs2Id)
{
  writeKdmActionRelation(KdmType::Reads(), actionId, rhs1Id);
  writeKdmActionRelation(KdmType::Reads(), actionId, rhs2Id);
  writeKdmActionRelation(KdmType::Writes(), actionId, lhsId);
}

} // namespace kdmtriplewriter

} // namespace gcckdm
