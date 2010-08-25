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
    mFunctionEntryData.reset();

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
  return mKdmWriter.getReferenceId(ast);
}


GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::getRhsReferenceId(tree const rhs)
{
  ActionDataPtr data;

  //Value types we handle special have to add them to languageUnit
  if (isValueNode(rhs))
  {
    data = ActionDataPtr(new ActionData());
    data->outputId(getReferenceId(rhs));
    mKdmWriter.processAstNode(rhs);
  }
  else if (TREE_CODE(rhs) == ADDR_EXPR)
  {
    data = writeKdmPtrParam(rhs, gcckdm::locationOf(rhs));
  }
  else if (TREE_CODE(rhs) == ARRAY_REF)
  {
    data = writeKdmArraySelect(NULL_TREE, rhs, gcckdm::locationOf(rhs), true);
  }
  else if (TREE_CODE(rhs) == COMPONENT_REF)
  {
    data = writeKdmMemberSelect(NULL_TREE, rhs, gcckdm::locationOf(rhs), true);
  }
  else
  {
    data = ActionDataPtr(new ActionData());
    data->outputId(getReferenceId(rhs));
  }
  return data;
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
  ActionDataPtr actionData;
  if (gs)
  {
    switch (gimple_code(gs))
    {
      case GIMPLE_ASSIGN:
      {
        actionData = processGimpleAssignStatement(gs);
        break;
      }
      case GIMPLE_BIND:
      {
        processGimpleBindStatement(gs);
        break;
      }
      case GIMPLE_CALL:
      {
        actionData = processGimpleCallStatement(gs);
        break;
      }
      case GIMPLE_COND:
      {
        actionData = processGimpleConditionalStatement(gs);
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
        actionData = processGimpleReturnStatement(gs);
        break;
      }
      case GIMPLE_SWITCH:
      {
        actionData = processGimpleSwitchStatement(gs);
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
    if (actionData and not mFunctionEntryData)
    {
      writeKdmActionRelation(KdmType::EntryFlow(), mCurrentCallableUnitId, actionData->startActionId());
      mFunctionEntryData = actionData;
    }

    //After the first action element we need to hook up the flows
    if (mLastData and actionData)
    {
      writeKdmActionRelation(KdmType::Flow(), mLastData->actionId(), actionData->startActionId());
    }

    //the gimple we just processed returned an action remember it
    if (actionData)
    {
      mLastData = actionData;
    }

    // If the last gimple statement we processed was a label or some goto's
    // we have to do a little magic here to get the flows
    // correct... some labels/goto's don't have line numbers so we add
    // the label to the next actionElement after the label
    // mLabelFlag is set in the processGimpleLabelStatement and processGimpleGotoStatement methods
    //
    if (mLabelFlag and actionData)
    {
      long blockId = getBlockReferenceId(gimple_location(gs));
      mKdmWriter.writeTripleContains(blockId, mLastLabelData->actionId());
      writeKdmActionRelation(KdmType::Flow(), mLastLabelData->actionId(), actionData->actionId());
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

GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::processGimpleAssignStatement(gimple const gs)
{
  ActionDataPtr actionData;
  unsigned numOps(gimple_num_ops(gs));
  if (numOps == 2)
  {
    actionData = processGimpleUnaryAssignStatement(gs);
  }
  else if (numOps == 3)
  {
    actionData = processGimpleBinaryAssignStatement(gs);
  }
  else if (numOps == 4)
  {
    actionData = processGimpleTernaryAssignStatement(gs);
  }
  else
  {
    mKdmWriter.writeComment("GimpleKdmTripleWriter::processGimpleAssignStatement: Unsupported number of operations");
  }

  if (actionData)
  {
    //This action element is contained in a block unit
    long blockId = getBlockReferenceId(gimple_location(gs));
    mKdmWriter.writeTripleContains(blockId, actionData->actionId());
  }
  return actionData;
}

GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::processGimpleReturnStatement(gimple const gs)
{
  long actionId = mKdmWriter.getNextElementId();
  ActionDataPtr actionData(new ActionData(actionId));

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

  return actionData;
}

GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::processGimpleLabelStatement(gimple const gs)
{
  tree label = gimple_label_label(gs);
  mLastLabelData = writeKdmNopForLabel(label);
  mLabelFlag = true;
  return mLastLabelData;
}

GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmNopForLabel(tree const label)
{
  long actionId = getReferenceId(label);
  ActionDataPtr actionData(new ActionData(actionId));
  mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionId, KdmKind::Nop());
  mKdmWriter.writeTripleName(actionId, gcckdm::getAstNodeName(label));
  return actionData;
}





//GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::updateActionFlow(ActionDataPtr actionData, long const actionId)
//{
//  ActionDataPtr newFlow;
//
//  if (actionData)
//  {
//    actionData->end = actionId;
//    newFlow = ActionDataPtr(new Flow(*actionData));
//  }
//  else
//  {
//    newFlow = ActionDataPtr(new Flow(actionId));
//  }
//  return newFlow;
//}
//
//GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::updateFlow(ActionDataPtr mainFlow, ActionDataPtr flowUpdate)
//{
//  if (flowUpdate->isComplex())
//  {
//    if (mainFlow->hasEndPoints())
//    {
//      writeKdmActionRelation(KdmType::Flow(), mainFlow->end, flowUpdate->start);
//      mainFlow->end(flowUpdate->end());
//    }
//    else
//    {
//      mainFlow->start(flowUpdate->start());
//      mainFlow->end(flowUpdate->end());
//    }
//  }
//  else
//  {
//    return mainFlow;
//  }
//}

//GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::updateFlow(ActionDataPtr mainFlow, ActionDataPtr flowUpdate)
//{
//  ActionDataPtr newFlow;
//
//  if (flowUpdate->valid)
//  {
//    if (mainFlow)
//    {
//      writeKdmActionRelation(KdmType::Flow(), mainFlow->end, flowUpdate->start);
//      newFlow = ActionDataPtr(new Flow(mainFlow->start, flowUpdate->end, flowUpdate->valid));
//    }
//  }
//  else
//  {
//    if (mainFlow)
//    {
//      newFlow = ActionDataPtr(new Flow(*mainFlow));
//    }
//  }
//  return newFlow;
//}

GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::processGimpleConditionalStatement(gimple const gs)
{
  //Reserve an id
  long actionId = mKdmWriter.getNextElementId();
  ActionDataPtr actionData(new ActionData(actionId));

  //write the basic condition attributes
  mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionId, KdmKind::Condition());
  mKdmWriter.writeTripleName(actionId, "if");

  //get reference id's for the gimple condition
  long rhs1Id = getReferenceId(gimple_cond_lhs (gs));
  ActionDataPtr rhs2Data = getRhsReferenceId(gimple_cond_rhs(gs));

  //possibly update the actionData
  if (rhs2Data->hasActionId())
  {
    actionData->startActionId(rhs2Data->actionId());
  }

  //Create a boolean variable to store the result of the upcoming comparison
  long boolId = writeKdmStorableUnit(mKdmWriter.getUserTypeId(KdmType::BooleanType()), gimple_location(gs) );
  mKdmWriter.writeTripleContains(actionId, boolId);
  //store reference to created variable
  actionData->outputId(boolId);

  //Write the action Element containing the comparison... lessthan, greaterthan etc. etc.
  long condId = mKdmWriter.getNextElementId();
  mKdmWriter.writeTripleKdmType(condId, KdmType::ActionElement());
  mKdmWriter.writeTripleKind(condId, treeCodeToKind.find(gimple_cond_code (gs))->second);
  writeKdmBinaryRelationships(condId, boolId, rhs1Id, rhs2Data->outputId());

  //Write Flow from rhs of the condition if required
  if (rhs2Data->hasActionId())
  {
    writeKdmActionRelation(KdmType::Flow(), rhs2Data->actionId(), condId);
  }

  //Write location of the comparison as the same as the gimple condition statement
  long boolBlockId = getBlockReferenceId(gimple_location(gs));
  mKdmWriter.writeTripleContains(boolBlockId, condId);

  // Write read for the if
  writeKdmActionRelation(KdmType::Reads(), actionId, boolId);

  // Write flow for the if from comparison to if
  writeKdmActionRelation(KdmType::Flow(), condId, actionId);

  if (gimple_cond_true_label(gs))
  {
    tree trueNode(gimple_cond_true_label(gs));
    long trueFlowId(mKdmWriter.getNextElementId());
    long trueNodeId(getReferenceId(trueNode));

    mKdmWriter.writeTripleKdmType(trueFlowId, KdmType::TrueFlow());
    mKdmWriter.writeTriple(trueFlowId, KdmPredicate::From(), actionId);
    mKdmWriter.writeTriple(trueFlowId, KdmPredicate::To(), trueNodeId);
    mKdmWriter.writeTripleContains(actionId, trueFlowId);
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
  }

  long blockId = getBlockReferenceId(gimple_location(gs));
  mKdmWriter.writeTripleContains(blockId, actionId);

  return actionData;
}

GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::processGimpleCallStatement(gimple const gs)
{
  long actionId(mKdmWriter.getNextElementId());
  ActionDataPtr actionData(new ActionData(actionId));

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
      ActionDataPtr paramData;
      //parameter to a function is taking the address of something
      if (TREE_CODE(callArg) == ADDR_EXPR)
      {
        paramData = writeKdmPtrParam(callArg, gs);
      }
      else if (TREE_CODE(callArg) == COMPONENT_REF)
      {
        paramData = writeKdmMemberSelectParam(callArg, gs);
      }
      else
      {
        paramData = ActionDataPtr(new ActionData());
        paramData->outputId(getReferenceId(callArg));
      }

      writeKdmActionRelation(KdmType::Reads(), actionId, paramData->outputId());
//      if (not flow and paramFlow->valid)
//      {
//          flow = paramFlow;
//      }
//      else if (flow)
//      {
//        flow->end = paramFlow->end;
//      }
//      else
//      {
//        //do nothing
//      }
    }
  }
  else
  {
//    flow = ActionDataPtr(new Flow(actionId));
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

  return actionData;
}


GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::processGimpleGotoStatement(gimple const gs)
{
  long actionId(mKdmWriter.getNextElementId());
  ActionDataPtr actionData(new ActionData(actionId));

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
    mLastLabelData = actionData;
    mLabelFlag = true;
  }
  else
  {
    long blockId = getBlockReferenceId(gimple_location(gs));
    mKdmWriter.writeTripleContains(blockId, actionId);
  }
  return actionData;
}


GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::processGimpleUnaryAssignStatement(gimple const gs)
{
  ActionDataPtr actionData;
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
        actionData = writeKdmPtr(gs);
        break;
      }
      else
      {
        //Simple cast, no pointers or references
        mKdmWriter.writeComment("FIXME: This Assign is really a cast, but we do not support casts");
        actionData = writeKdmUnaryOperation(KdmKind::Assign(), gs);
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
          || gimpleRhsCode == SSA_NAME)
      {
        tree lhs = gimple_assign_lhs(gs);
        if (TREE_CODE(lhs) == INDIRECT_REF)
        {
          actionData = writeKdmPtrReplace(gs);
        }
        else if (TREE_CODE(lhs) == COMPONENT_REF)
        {
          actionData = writeKdmMemberReplace(gs);
        }
        else if (TREE_CODE(lhs) == ARRAY_REF)
        {
          actionData = writeKdmArrayReplace(gs);
        }
        else
        {
          actionData = writeKdmUnaryOperation(KdmKind::Assign(), gs);
        }
        break;
      }
      else if (gimpleRhsCode == CONSTRUCTOR)
      {
        actionData = writeKdmUnaryConstructor(gs);
      }
      else if (TREE_CODE_CLASS(gimpleRhsCode) == tcc_reference)
      {
        if (gimpleRhsCode == ARRAY_REF)
        {
          actionData = writeKdmArraySelect(gs);
        }
        else if(gimpleRhsCode == COMPONENT_REF)
        {
          actionData = writeKdmMemberSelect(gs);
          break;
        }
        else if (gimpleRhsCode == INDIRECT_REF)
        {
          actionData = writeKdmPtr(gs);
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
        actionData = writeKdmPtr(gs);
        break;
      }
      else if (gimpleRhsCode == NEGATE_EXPR)
      {
        actionData = writeKdmUnaryOperation(KdmKind::Negate(), gs);
        break;
      }
      else if (gimpleRhsCode == BIT_NOT_EXPR)
      {
        actionData = writeKdmUnaryOperation(KdmKind::BitNot(), gs);
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
  return actionData;
}




GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::processGimpleBinaryAssignStatement(gimple const gs)
{
  ActionDataPtr actionData;
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
            actionData = writeKdmBinaryOperation(treeCodeToKind.find(gimple_assign_rhs_code(gs))->second, gs);
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
  return actionData;
}

GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::processGimpleTernaryAssignStatement(gimple const gs)
{
  enum tree_code rhs_code = gimple_assign_rhs_code(gs);
  std::string msg(boost::str(boost::format("GIMPLE assignment operation (%1%) in %2%") % std::string(tree_code_name[rhs_code])
  % BOOST_CURRENT_FUNCTION));
  mKdmWriter.writeUnsupportedComment(msg);
  return ActionDataPtr();
}


GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::processGimpleSwitchStatement(gimple const gs)
{
  long actionId = mKdmWriter.getNextElementId();
  ActionDataPtr actionData(new ActionData(actionId));

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
    }
  }
  return actionData;
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

GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmUnaryConstructor(gimple const gs)
{
  tree lhs(gimple_assign_lhs(gs));
  tree rhs(gimple_assign_rhs1(gs));

  long actionId = mKdmWriter.getNextElementId();
  long lhsId = getReferenceId(lhs);

  location_t loc = gimple_location(gs);
  long tmpId = writeKdmStorableUnit(getReferenceId(TREE_TYPE(rhs)), loc);
  const long blockUnitId(getBlockReferenceId(loc));
  mKdmWriter.writeTripleContains(blockUnitId, tmpId);
  ActionDataPtr rhsData= ActionDataPtr(new ActionData());
  rhsData->outputId(tmpId);

  mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionId, KdmKind::Assign());
  writeKdmUnaryRelationships(actionId, lhsId, tmpId);

  return rhsData;
}

GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmUnaryOperation(KdmKind const & kind, gimple const gs)
{
  tree lhs(gimple_assign_lhs(gs));
  tree rhs(gimple_assign_rhs1(gs));
  return writeKdmUnaryOperation(kind, lhs, rhs);
}


GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmUnaryOperation(KdmKind const & kind, tree const lhs, tree const rhs)
{
  long actionId = mKdmWriter.getNextElementId();
  ActionDataPtr actionData(new ActionData(actionId));
  long lhsId = getReferenceId(lhs);
  ActionDataPtr rhsData = getRhsReferenceId(rhs);

  if (rhsData->hasActionId())
  {
    writeKdmActionRelation(KdmType::Flow(), rhsData->actionId(), actionId);
    actionData->startActionId(rhsData->actionId());
  }

  mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionId, kind);

  writeKdmUnaryRelationships(actionId, lhsId, rhsData->getTargetId());
  return actionData;
}


GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmPtrReplace(gimple const gs)
{
  tree lhs = gimple_assign_lhs(gs);
  tree rhs = gimple_assign_rhs1(gs);
  long actionId = mKdmWriter.getNextElementId();
  ActionDataPtr actionData(new ActionData(actionId));
  tree lhsOp0 = TREE_OPERAND (lhs, 0);
  long lhsId = getReferenceId(lhsOp0);
  ActionDataPtr rhsData = getRhsReferenceId(rhs);
  if (rhsData->hasActionId())
  {
    writeKdmActionRelation(KdmType::Flow(), rhsData->actionId(), actionId);
    actionData->startActionId(rhsData->actionId());
  }

  mKdmWriter.writeTripleKind(actionId, KdmKind::PtrReplace());
  writeKdmActionRelation(KdmType::Reads(), actionId, rhsData->getTargetId());
  writeKdmActionRelation(KdmType::Addresses(), actionId, lhsId);

  //Have to determine if there is a bug in the spec here or not
  //where does the write relationship go?
  mKdmWriter.writeComment("FIXME: KDM spec states there should be a writes relationship here.. ");
  //writeKdmActionRelation(KdmType::Writes(), actionId, lhsId);
  //return updateActionFlow(actionData, actionId);
  return actionData;
}

GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmBinaryOperation(KdmKind const & kind, gimple const gs)
{
  tree lhs(gimple_assign_lhs(gs));
  tree rhs1(gimple_assign_rhs1(gs));
  tree rhs2(gimple_assign_rhs2(gs));
  return writeKdmBinaryOperation(kind, lhs, rhs1, rhs2);
}

GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmBinaryOperation(KdmKind const & kind, tree const lhs, tree const rhs1, tree const rhs2)
{
  long actionId = mKdmWriter.getNextElementId();
  ActionDataPtr actionData(new ActionData(actionId));
  mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionId, kind);

  ActionDataPtr rhs1Data = getRhsReferenceId(rhs1);

  if (rhs1Data->hasActionId())
  {
    writeKdmActionRelation(KdmType::Flow(), rhs1Data->actionId(), actionId);
    actionData->startActionId(rhs1Data->actionId());
  }

  ActionDataPtr rhs2Data = getRhsReferenceId(rhs2);
  if (rhs2Data->hasActionId())
  {
    writeKdmActionRelation(KdmType::Flow(), rhs2Data->actionId(), actionId);
    actionData->startActionId(rhs2Data->actionId());
  }

  long lhsId = getReferenceId(lhs);
  writeKdmBinaryRelationships(actionId, lhsId, rhs1Data->getTargetId(), rhs2Data->getTargetId());
  //return updateActionFlow(actionData, actionId);
  return actionData;
}


//D.1989 = a[23];
GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmArraySelect(gimple const gs)
{
  tree lhs = gimple_assign_lhs(gs);
  tree rhs = gimple_assign_rhs1(gs);
  return writeKdmArraySelect(lhs, rhs, gimple_location(gs), false);
}


GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmArraySelect(tree const lhs, tree const rhs, location_t const loc, bool writeBlockUnit)
{
  long dummy;
  return writeKdmArraySelect(lhs, rhs, loc, writeBlockUnit, dummy);
}

GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmArraySelect(tree const lhs, tree const rhs, location_t const loc, bool writeBlockUnit, long & tmpId)
{
  assert(TREE_CODE(rhs) == ARRAY_REF);

  ActionDataPtr arraySelectFlow;

  long actionId(mKdmWriter.getNextElementId());
  ActionDataPtr actionData(new ActionData(actionId));
  expanded_location xloc = expand_location(loc);
  //var_decl
  tree op0 = TREE_OPERAND (rhs, 0);
  //index
  tree op1 = TREE_OPERAND (rhs, 1);
  ActionDataPtr op0Data = getRhsReferenceId(op0);
  if (op0Data->hasActionId())
  {
    writeKdmActionRelation(KdmType::Flow(), op0Data->actionId(), actionId);
    actionData->startActionId(op0Data->actionId());
  }
// arraySelectFlow = updateFlow(arraySelectFlow, op0Data);

  ActionDataPtr op1Data = getRhsReferenceId(op1);
// arraySelectFlow = updateFlow(arraySelectFlow, op1Data);
  if (op1Data->hasActionId())
  {
    writeKdmActionRelation(KdmType::Flow(), op1Data->actionId(), actionId);
    if (actionData->startActionId() != ActionData::InvalidId)
    {
      actionData->startActionId(op1Data->actionId());
    }
  }

  mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionId, KdmKind::ArraySelect());
  writeKdmActionRelation(KdmType::Reads(), actionId, op1Data->getTargetId());
  writeKdmActionRelation(KdmType::Addresses(), actionId, op0Data->getTargetId());

  long lhsId;
  if (lhs == NULL_TREE)
  {
    lhsId = writeKdmStorableUnit(getReferenceId(TREE_TYPE(op0)),xloc);
    tmpId = lhsId;
  }
  else
  {
    lhsId = getReferenceId(lhs);
  }
  mKdmWriter.writeTripleContains(actionId, lhsId);
  writeKdmActionRelation(KdmType::Writes(), actionId, lhsId);
  actionData->outputId(lhsId);

  if (writeBlockUnit)
  {
    const long blockUnitId(getBlockReferenceId(loc));
    mKdmWriter.writeTripleContains(blockUnitId, lhsId);
  }
  return actionData;

}

// foo[0] = 1
GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmArrayReplace(gimple const gs)
{
  long actionId = mKdmWriter.getNextElementId();
  ActionDataPtr actionData(new ActionData(actionId));

  tree lhs = gimple_assign_lhs(gs);
  tree rhs = gimple_assign_rhs1(gs);
  //var_decl
  tree op0 = TREE_OPERAND (lhs, 0);
  //index
  tree op1 = TREE_OPERAND (lhs, 1);

  ActionDataPtr rhsData = getRhsReferenceId(rhs);
  long lastId = ActionData::InvalidId;
  if (rhsData->hasActionId())
  {
    actionData->startActionId(rhsData->actionId());
    lastId = rhsData->actionId();
  }

  ActionDataPtr selectData;
  //long tmpId;
  while (TREE_CODE(op0) == ARRAY_REF)
  {
    selectData = writeKdmArraySelect(NULL_TREE, op0, gimple_location(gs), true);

    if (actionData->startActionId() == ActionData::InvalidId)
    {
      actionData->startActionId(selectData->actionId());
    }
    else
    {
      writeKdmActionRelation(KdmType::Flow(), lastId ,selectData->actionId());
    }
    lastId = selectData->actionId();

    op1 = TREE_OPERAND (op0, 1);
    op0 = TREE_OPERAND (op0, 0);
  }

  long op0Id = (selectData) ? selectData->outputId() :getReferenceId(op0);
  long op1Id = getReferenceId(op1);

  mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionId, KdmKind::ArrayReplace());
  writeKdmActionRelation(KdmType::Addresses(), actionId, op0Id); //data element
  writeKdmActionRelation(KdmType::Reads(), actionId, op1Id); // index
  writeKdmActionRelation(KdmType::Reads(), actionId, rhsData->getTargetId()); //new value

  //Have to determine if there is a bug in the spec here or not
  //where does the write relationship go?
  mKdmWriter.writeComment("FIXME: KDM spec states there should be a writes relationship here..");
  return actionData;
}


GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmMemberSelect(tree const lhs, tree const rhs, location_t const loc, bool writeBlockUnit)
{
  assert(TREE_CODE(rhs) == COMPONENT_REF);

  expanded_location xloc = expand_location(loc);
  tree op0 = TREE_OPERAND (rhs, 0);
  tree op1 = TREE_OPERAND (rhs, 1);
  ActionDataPtr actionData(new ActionData());
  long lhsId;

  if (TREE_CODE(op0) == INDIRECT_REF)
  {
    //we have to create a temp variable to hold the Ptr result
    tree indirectRef = TREE_OPERAND (op0, 0);
    long refId = getReferenceId(indirectRef);
    lhsId = writeKdmStorableUnit(getReferenceId(TREE_TYPE(indirectRef)),loc);

    //Resolve the indirect reference put result in temp
    ActionDataPtr ptrData = writeKdmPtr(lhsId, refId);
    actionData->startActionId(ptrData->actionId());
    mKdmWriter.writeTripleContains(ptrData->outputId(), lhsId);

    //perform memberselect using temp
    ActionDataPtr op1Data = getRhsReferenceId(op1);
    //actionData = updateFlow(actionData, op1Data);
    if (op1Data->hasActionId())
    {
      writeKdmActionRelation(KdmType::Flow(), ptrData->actionId(), op1Data->actionId());
    }


    ActionDataPtr memberSelectData = writeKdmMemberSelect(lhsId, op1Data->getTargetId(), ptrData->getTargetId());
    mKdmWriter.writeTripleContains(memberSelectData->actionId(), ptrData->getTargetId());
    //actionData = updateFlow(actionData, memberSelectData);
    writeKdmActionRelation(KdmType::Flow(), op1Data->getTargetId(), op1Data->actionId());


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

    ActionDataPtr op0Data(getRhsReferenceId(op0));
    //actionData = updateFlow(ActionDataPtr(), op0Data);
    if (op0Data->hasActionId())
    {
      actionData->startActionId(op0Data->actionId());
    }

    ActionDataPtr op1Data(getRhsReferenceId(op1));
    if (op0Data->hasActionId() && actionData->startActionId() == ActionData::InvalidId)
    {
      actionData->startActionId(op0Data->actionId());
    }
    else if (op0Data->hasActionId())
    {
      writeKdmActionRelation(KdmType::Flow(), op0Data->hasActionId(), op1Data->actionId());
    }

    ActionDataPtr memberData = writeKdmMemberSelect(lhsId, op1Data->getTargetId(), op0Data->getTargetId());
    //actionData = updateActionFlow(actionData, memberData->getTargetId());
    actionData->outputId(memberData->outputId());

  }
  return actionData;
}

GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmMemberSelect(long const writesId, long const readsId, long const invokesId)
{
  long actionId = mKdmWriter.getNextElementId();
  ActionDataPtr actionData(new ActionData(actionId));
  mKdmWriter.writeTripleKind(actionId, KdmKind::MemberSelect());
  writeKdmActionRelation(KdmType::Reads(), actionId, readsId);
  writeKdmActionRelation(KdmType::Invokes(), actionId, invokesId);
  writeKdmActionRelation(KdmType::Writes(), actionId, writesId);
  return actionData;
}

/** Write to LHS
 * Addresses object
 * Reads member
 *
 */
//D.1716 = this->m_bar;
//D.4427 = hp->h_length;
GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmMemberSelect(gimple const gs)
{
  tree lhs = gimple_assign_lhs(gs);
  tree rhs = gimple_assign_rhs1(gs);
  tree op0 = TREE_OPERAND (rhs, 0);
  tree op1 = TREE_OPERAND (rhs, 1);
  long lhsId = getReferenceId(lhs);

  ActionDataPtr actionData;

  if (TREE_CODE(op0) == INDIRECT_REF)
  {
    tree indirectRef = TREE_OPERAND (op0, 0);
    long refId = getReferenceId(indirectRef);
    ActionDataPtr op1Data = getRhsReferenceId(op1);
//    actionData = updateFlow(ActionDataPtr(), op1Data);

    //we have to create a temp variable to hold the Ptr result
    long storableId = writeKdmStorableUnit(getReferenceId(TREE_TYPE(indirectRef)),expand_location(gimple_location(gs)));

    //Resolve the indirect reference put result in temp
    long ptrActionId = mKdmWriter.getNextElementId();
    mKdmWriter.writeTripleKdmType(ptrActionId, KdmType::ActionElement());
    mKdmWriter.writeTripleKind(ptrActionId, KdmKind::Ptr());
    writeKdmActionRelation(KdmType::Addresses(), ptrActionId, refId);
    writeKdmActionRelation(KdmType::Writes(), ptrActionId, storableId);
    mKdmWriter.writeTripleContains(ptrActionId, storableId);
   // actionData = updateActionFlow(actionData, ptrActionId);


    //perform memberselect using temp
    long actionId = mKdmWriter.getNextElementId();
    mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
    mKdmWriter.writeTripleKind(actionId, KdmKind::MemberSelect());
    writeKdmActionRelation(KdmType::Reads(), actionId, op1Data->getTargetId());
    writeKdmActionRelation(KdmType::Invokes(), actionId, ptrActionId);
    writeKdmActionRelation(KdmType::Writes(), actionId, lhsId);
    mKdmWriter.writeTripleContains(actionId, ptrActionId);
    //actionData = updateActionFlow(actionData, actionId);
  }
  else
  {
    long actionId = mKdmWriter.getNextElementId();
    mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
    mKdmWriter.writeTripleKind(actionId, KdmKind::MemberSelect());
    ActionDataPtr op0Data = getRhsReferenceId(op0);
//    actionData = updateFlow(ActionDataPtr(), op0Data);
    ActionDataPtr op1Data = getRhsReferenceId(op1);
//    actionData = updateFlow(ActionDataPtr(), op1Data);

    writeKdmActionRelation(KdmType::Reads(), actionId, op1Data->getTargetId());
    writeKdmActionRelation(KdmType::Invokes(), actionId, op0Data->getTargetId());
    writeKdmActionRelation(KdmType::Writes(), actionId, lhsId);
    //actionData = updateActionFlow(actionData, actionId);
  }
  return actionData;
}

GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmMemberSelectParam(tree const compRef, gimple const gs)
{
  return writeKdmMemberSelectParam(compRef, gimple_location(gs));
}

GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmMemberSelectParam(tree const compRef, location_t const loc)
{
  const long blockUnitId(getBlockReferenceId(loc));
  tree op0 = TREE_OPERAND (compRef, 0);
  tree op1 = TREE_OPERAND (compRef, 1);

  long actionId = mKdmWriter.getNextElementId();
  ActionDataPtr actionData;

  mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionId, KdmKind::MemberSelect());

  //we have to create a temp variable
  long storableId = writeKdmStorableUnit(getReferenceId(TREE_TYPE(op0)), loc);
  mKdmWriter.writeTripleContains(blockUnitId, storableId);

  ActionDataPtr op0Data = getRhsReferenceId(op0);
//  actionData = updateFlow(ActionDataPtr(), op0Data);
  ActionDataPtr op1Data = getRhsReferenceId(op1);
//  actionData = updateFlow(ActionDataPtr(), op1Data);

  writeKdmActionRelation(KdmType::Reads(), actionId, op1Data->getTargetId());
  writeKdmActionRelation(KdmType::Invokes(), actionId, op0Data->getTargetId());
  writeKdmActionRelation(KdmType::Writes(), actionId, storableId);
  mKdmWriter.writeTripleContains(blockUnitId, actionId);
  //return updateActionFlow(actionData, actionId);
  return actionData;
}


//Example: sin.sin_family = 2;
GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmMemberReplace(gimple const gs)
{
  tree lhs = gimple_assign_lhs(gs);
  tree rhs = gimple_assign_rhs1(gs);
  tree lhsOp0 = TREE_OPERAND (lhs, 0);
  long lhsId = getReferenceId(lhsOp0);
  ActionDataPtr rhsData = getRhsReferenceId(rhs);
  //ActionDataPtr actionData = updateFlow(ActionDataPtr(), rhsData);
  long actionId = mKdmWriter.getNextElementId();
  ActionDataPtr actionData(new ActionData(actionId));

  mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionId, KdmKind::MemberReplace());
  writeKdmActionRelation(KdmType::Reads(), actionId, rhsData->getTargetId());
  writeKdmActionRelation(KdmType::Invokes(), actionId, lhsId);
  writeKdmActionRelation(KdmType::Writes(), actionId, lhsId);
  return actionData;
  //updateActionFlow(actionData, actionId);
}


//ptr = &a[0];
GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmPtr(gimple const gs)
{
  ActionDataPtr actionData;
  tree lhs = gimple_assign_lhs(gs);
  tree rhs = gimple_assign_rhs1(gs);
  tree op0 = TREE_OPERAND (rhs, 0);

  if (TREE_CODE(op0) == ARRAY_REF)
  {
    //Write Array Select Action Element into a temporary
    actionData = writeKdmArraySelect(NULL_TREE, op0, gimple_location(gs), true);
    ActionDataPtr ptrData = writeKdmPtr(getReferenceId(lhs), actionData->actionId());
    mKdmWriter.writeTripleContains(actionData->getTargetId(), ptrData->getTargetId());
    //actionData = updateActionFlow(actionData, ptrData->getTargetId());
  }
  else
  {
    ActionDataPtr rhsData = getRhsReferenceId(op0);
//    actionData = updateFlow(ActionDataPtr(), rhsData);
    ActionDataPtr ptrData = writeKdmPtr(getReferenceId(lhs),rhsData->getTargetId());
    //actionData = updateActionFlow(actionData, ptrData->getTargetId());
  }
  return actionData;
}

GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmPtr(long const writesId, long const addressesId)
{
  long actionId = mKdmWriter.getNextElementId();
  ActionDataPtr actionData(new ActionData(actionId));
  mKdmWriter.writeTripleKind(actionId, KdmKind::Ptr());
  writeKdmActionRelation(KdmType::Addresses(), actionId, addressesId);
  writeKdmActionRelation(KdmType::Writes(), actionId, writesId);
  actionData->outputId(writesId);
  return actionData;
}



GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmPtrParam(tree const addrExpr, gimple const gs)
{
  return writeKdmPtrParam(addrExpr, gimple_location(gs));
}

GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmPtrParam(tree const addrExpr, location_t const loc)
{
  //All of these elements are contained in the same block unit
  const long blockUnitId(getBlockReferenceId(loc));
  tree op0 = TREE_OPERAND (addrExpr, 0);
  ActionDataPtr actionData;

  if (TREE_CODE(op0) == ARRAY_REF)
  {
    tree refOp0 = TREE_OPERAND (op0, 0);
    ActionDataPtr selectData = writeKdmArraySelect(NULL_TREE, op0, loc, true);

    //we have to create a second temp variable... aka temp2
    long storable2Id = writeKdmStorableUnit(getReferenceId(TREE_TYPE(refOp0)),gcckdm::locationOf(addrExpr));
    mKdmWriter.writeTripleContains(blockUnitId, storable2Id);

    //perform address on temp1 assign to temp2    temp2 = &rhs
    ActionDataPtr ptrData = writeKdmPtr(storable2Id, selectData->getTargetId());
    mKdmWriter.writeTripleContains(blockUnitId, ptrData->getTargetId());
  }
  else if (TREE_CODE(op0) == COMPONENT_REF)
  {
    ActionDataPtr selectFlow = writeKdmMemberSelectParam(op0, loc);
    //actionData = updateActionFlow(actionData, selectFlow->getTargetId());
    mKdmWriter.writeTripleContains(blockUnitId, actionData->getTargetId());
  }
  else
  {
    //we have to create a temp variable... aka temp1
    long storableId = writeKdmStorableUnit(getReferenceId(TREE_TYPE(TREE_OPERAND (addrExpr, 0))),gcckdm::locationOf(addrExpr));
    mKdmWriter.writeTripleContains(blockUnitId, storableId);

    //Address.... temp1 = &rhs
    ActionDataPtr rhsData = getRhsReferenceId(op0);
    ActionDataPtr ptrData = writeKdmPtr(storableId, rhsData->outputId());

//    if (rhsData->hasActionId())
//    {
//      writeKdmActionRelation(KdmType::Flow(), rhsData, ptrData->actionId());
//    }
    mKdmWriter.writeTripleContains(blockUnitId, ptrData->actionId());
//    writeKdmActionRelation(KdmType::Flow(), ptrData->getTargetId(), actionId);

  }
  return actionData;
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
