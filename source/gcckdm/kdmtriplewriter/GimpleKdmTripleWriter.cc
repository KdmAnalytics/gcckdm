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
                              (TRUNC_DIV_EXPR, gcckdm::KdmKind::Divide())
                              (EXACT_DIV_EXPR, gcckdm::KdmKind::Divide())
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

/**
 * Returns true is the TREE_CODE of the given node is a value aka Integer, real or string
 */
bool isValueNode(tree const node)
{
  return TREE_CODE(node) == INTEGER_CST || TREE_CODE(node) == REAL_CST || TREE_CODE(node) == STRING_CST;
}

} // namespace

namespace gcckdm
{

namespace kdmtriplewriter
{

GimpleKdmTripleWriter::GimpleKdmTripleWriter(KdmTripleWriter & tripleWriter) :
      mKdmWriter(tripleWriter),
      mLabelFlag(false),
      mRegisterVariableIndex(0)
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
    mFunctionEntryData.reset();
    mLastData.reset();
    //Labels are only unique within a function, have to clear map for each function or
    //we get double containment if the user used the same label in to functions
    mLabelMap.clear();

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
      //Fall Through
    case PARM_DECL:
      //Fall Through
    case FUNCTION_DECL:
    {
      //Do nothing
      break;
    }
    case INDIRECT_REF:
      //Fall Through
    case NOP_EXPR:
      //Fall Through
    case ADDR_EXPR:
    {
      //Recursively call ourselves
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
  if (TREE_CODE(ast) == COMPONENT_REF)
  {
    int i  = 0;
  }
  if (TREE_CODE(ast) == INDIRECT_REF)
  {
    int i = 0;
  }
  if (TREE_CODE(ast) == ADDR_EXPR)
  {
    int i = 0;
  }

  //Labels are apparently reused causing different
  //labels to be referenced as the same label which
  //causes double containment problems
  //we use the name of the label instead
  if (TREE_CODE(ast) == LABEL_DECL)
  {
    std::string labelName = gcckdm::getAstNodeName(ast);
    LabelMap::const_iterator i = mLabelMap.find(labelName);
    if (i == mLabelMap.end())
    {
      std::pair<LabelMap::iterator, bool> result = mLabelMap.insert(std::make_pair(labelName, mKdmWriter.getNextElementId()));
      return result.first->second;
    }
    else
    {
      return i->second;
    }
  }
  else
  {
    return mKdmWriter.getReferenceId(ast);
  }
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
    data = writeKdmPtr(NULL_TREE, rhs, gcckdm::locationOf(rhs));
  }
  else if (TREE_CODE(rhs) == ARRAY_REF)
  {
    data = writeKdmArraySelect(NULL_TREE, rhs, gcckdm::locationOf(rhs), true);
  }
  else if (TREE_CODE(rhs) == COMPONENT_REF)
  {
    data = writeKdmMemberSelect(NULL_TREE, rhs, gcckdm::locationOf(rhs));
  }
  else if (TREE_CODE(rhs) == INDIRECT_REF)
  {
    data = writeKdmPtrSelect(NULL_TREE, rhs, gcckdm::locationOf(rhs));
  }
  else if (TREE_CODE(rhs) == BIT_FIELD_REF)
  {
    data = writeBitAssign(NULL_TREE, rhs, gcckdm::locationOf(rhs));
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
      case GIMPLE_ASM:
      {
        actionData = processGimpleAsmStatement(gs);
        break;
      }
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
      case GIMPLE_PREDICT:
      {
        //This statement doesn't appear to have any relevance to KDM
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
      for (; !mLabelQueue.empty(); mLabelQueue.pop())
      {
        mKdmWriter.writeTripleContains(blockId, mLabelQueue.front()->actionId());
        writeKdmActionRelation(KdmType::Flow(), mLabelQueue.front()->actionId(), actionData->actionId());
      }
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
    // NGINX has LABEL_DECLS in the bind vars... don't think we need them
    // so we are skipping them until we actually know what they are for
    if (TREE_CODE(var) == LABEL_DECL)
    {
      mKdmWriter.writeComment("Skipping label decl in bind statement....");
      continue;
    }

    if (TREE_CODE(var) == TYPE_DECL)
    {
      //User has declared a type within a function so we skip it currently
      //to prevent double containment when it is output at the
      //end of the translation unit
    }
    else
    {
      long declId = getReferenceId(var);
      mKdmWriter.processAstNode(var);
      mKdmWriter.writeTripleContains(mCurrentCallableUnitId, declId);
    }
  }
  processGimpleSequence(gimple_bind_body(gs));
  mKdmWriter.writeComment("================GIMPLE END BIND STATEMENT " + gcckdm::getAstNodeName(mCurrentFunctionDeclarationNode) + "==========================");
}

GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::processGimpleAsmStatement(gimple const gs)
{
  ActionDataPtr actionData(new ActionData(mKdmWriter.getNextElementId()));
  mKdmWriter.writeTripleKdmType(actionData->actionId(), KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionData->actionId(), KdmKind::Asm());
  mKdmWriter.writeTripleName(actionData->actionId(), gimple_asm_string (gs));

  unsigned int fields = 0;

  if (gimple_asm_nlabels (gs))
  {
    fields = 4;
  }
  else if (gimple_asm_nclobbers (gs))
  {
    fields = 3;
  }
  else if (gimple_asm_ninputs (gs))
  {
    fields = 2;
  }
  else if (gimple_asm_noutputs (gs))
  {
    fields = 1;
  }
  unsigned n;
  for (unsigned f = 0; f < fields; ++f)
  {
    switch (f)
    {
      case 0:
      {
        n = gimple_asm_noutputs (gs);
        if (n != 0)
        {
          for (unsigned i = 0; i < n; i++)
          {
            long id = getReferenceId(gimple_asm_output_op (gs, i));
            writeKdmActionRelation(KdmType::Writes(), actionData->actionId(), id);
          }
        }
        break;
      }
      case 1:
      {
        n = gimple_asm_ninputs (gs);
        if (n != 0)
        {
          for (unsigned i = 0; i < n; i++)
          {
            long id = getReferenceId(gimple_asm_input_op (gs, i));
            writeKdmActionRelation(KdmType::Reads(), actionData->actionId(), id);
          }
        }
      }
      default:
      {
        //do nothing
      }
    }
  }
  return actionData;
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

  //If this statement is returning a value then we process it
  //and write a reads
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
  ActionDataPtr actionData = writeKdmNopForLabel(label);
  mLabelQueue.push(actionData);
//  mLastLabelData = writeKdmNopForLabel(label);
  mLabelFlag = true;
  return actionData;
}

GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmNopForLabel(tree const label)
{
  ActionDataPtr actionData(new ActionData(getReferenceId(label)));
  mKdmWriter.writeTripleKdmType(actionData->actionId(), KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionData->actionId(), KdmKind::Nop());
  mKdmWriter.writeTripleName(actionData->actionId(), gcckdm::getAstNodeName(label));
  return actionData;
}

GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::processGimpleConditionalStatement(gimple const gs)
{
  //Reserve an id
  ActionDataPtr actionData = ActionDataPtr(new ActionData(mKdmWriter.getNextElementId()));

  //write the basic condition attributes
  mKdmWriter.writeTripleKdmType(actionData->actionId(), KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionData->actionId(), KdmKind::Condition());
  mKdmWriter.writeTripleName(actionData->actionId(), "if");

  //get reference id's for the gimple condition
  //The condition can be an addr_expr or a var_decl
  //Examples:  &a == b;
  //            a == b;
  ActionDataPtr rhs1Data = getRhsReferenceId(gimple_cond_lhs (gs));
  ActionDataPtr rhs2Data = getRhsReferenceId(gimple_cond_rhs(gs));

  //Create a boolean variable to store the result of the upcoming comparison
  long boolId = writeKdmStorableUnit(mKdmWriter.getUserTypeId(KdmType::BooleanType()), gimple_location(gs) );
  mKdmWriter.writeTripleContains(actionData->actionId(), boolId);
  //store reference to created variable
  actionData->outputId(boolId);

  //Write the action Element containing the comparison... lessthan, greaterthan etc. etc.
  ActionDataPtr condData = ActionDataPtr(new ActionData(mKdmWriter.getNextElementId()));
  mKdmWriter.writeTripleKdmType(condData->actionId(), KdmType::ActionElement());
  mKdmWriter.writeTripleKind(condData->actionId(), treeCodeToKind.find(gimple_cond_code (gs))->second);
  writeKdmBinaryRelationships(condData->actionId(), boolId, rhs1Data->outputId(), rhs2Data->outputId());

  //Hook up flows and data
  configureDataAndFlow(condData,rhs1Data,rhs2Data);
  mKdmWriter.writeTripleContains(actionData->actionId(), condData->actionId());

  // Write read for the if
  writeKdmActionRelation(KdmType::Reads(), actionData->actionId(), boolId);

  // Write flow for the if from comparison to if
  writeKdmActionRelation(KdmType::Flow(), condData->actionId(), actionData->actionId());

  //Write true flow
  if (gimple_cond_true_label(gs))
  {
    tree trueNode(gimple_cond_true_label(gs));
    long trueFlowId(mKdmWriter.getNextElementId());
    long trueNodeId(getReferenceId(trueNode));

    mKdmWriter.writeTripleKdmType(trueFlowId, KdmType::TrueFlow());
    mKdmWriter.writeTriple(trueFlowId, KdmPredicate::From(), actionData->actionId());
    mKdmWriter.writeTriple(trueFlowId, KdmPredicate::To(), trueNodeId);
    mKdmWriter.writeTripleContains(actionData->actionId(), trueFlowId);
  }

  //Write false flow
  if (gimple_cond_false_label(gs))
  {
    tree falseNode(gimple_cond_false_label(gs));
    long falseFlowId(mKdmWriter.getNextElementId());
    long falseNodeId(getReferenceId(falseNode));
    mKdmWriter.writeTripleKdmType(falseFlowId, KdmType::FalseFlow());
    mKdmWriter.writeTriple(falseFlowId, KdmPredicate::From(), actionData->actionId());
    mKdmWriter.writeTriple(falseFlowId, KdmPredicate::To(), falseNodeId);
    mKdmWriter.writeTripleContains(actionData->actionId(), falseFlowId);
  }

  //Contain this action in a block unit
  long blockId = getBlockReferenceId(gimple_location(gs));
  mKdmWriter.writeTripleContains(blockId, actionData->actionId());

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
    ActionDataPtr lastParamData;
    for (size_t i = 0; i < gimple_call_num_args(gs); i++)
    {
      tree callArg(gimple_call_arg(gs, i));
      ActionDataPtr paramData;
      //parameter to a function is taking the address of something
      if (TREE_CODE(callArg) == ADDR_EXPR)
      {
        paramData = writeKdmPtr(NULL_TREE, callArg, gimple_location(gs));
      }
      else if (TREE_CODE(callArg) == COMPONENT_REF)
      {
        paramData = writeKdmMemberSelect(NULL_TREE, callArg, gimple_location(gs));
      }
      else if (TREE_CODE(callArg) == INDIRECT_REF)
      {
        paramData = writeKdmPtrSelect(NULL_TREE, callArg, gimple_location(gs));
      }
      else
      {
        paramData = ActionDataPtr(new ActionData());
        paramData->outputId(getReferenceId(callArg));
      }

      //Record the first param
      if (!actionData->hasStartAction())
      {
        actionData->startActionId(*paramData);
      }

      //Compound parameter?
      if (paramData->hasActionId())
      {
        //Contain this parameter within the call element
        mKdmWriter.writeTripleContains(actionId, paramData->actionId());

        //Hook up the flow if it's an action element and we have more than one param
        if (lastParamData)
        {
          writeKdmFlow(lastParamData->actionId(), paramData->actionId());
        }
        lastParamData = paramData;
      }

      //Hook up the read to each parameter
      writeKdmActionRelation(KdmType::Reads(), actionId, paramData->outputId());
    }

    //Write flow from the last parameter
    if (lastParamData)
    {
      writeKdmFlow(lastParamData->actionId(), actionId);
    }
  }

  //optional write
  // a = foo(b);
  tree lhs = gimple_call_lhs(gs);
  if (lhs)
  {
    long lhsId(getReferenceId(lhs));
    writeKdmActionRelation(KdmType::Writes(), actionId, lhsId);
  }

  //Contain this Call within a block unit
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
    //mLastLabelData = actionData;
    mLabelQueue.push(actionData);
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
  enum tree_code gimpleRhsCode = gimple_assign_rhs_code(gs);
  tree rhsNode = gimple_assign_rhs1(gs);
  switch (gimpleRhsCode)
  {
    case VIEW_CONVERT_EXPR:
      //Fall Through
    case ASSERT_EXPR:
    {
      std::string msg(boost::str(boost::format("GIMPLE assignment statement assert expression (%1%) in %2%:%3%") % std::string(tree_code_name[gimpleRhsCode]) % BOOST_CURRENT_FUNCTION % __LINE__));
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
    case ABS_EXPR:
    {
      actionData = writeKdmUnaryOperation(KdmKind::Assign(), gs);
      break;
    }
    case PAREN_EXPR:
       //Fall Through
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
        tree lhs = gimple_assign_lhs(gs);
        if (gimpleRhsCode == BIT_FIELD_REF)
        {
          actionData = writeBitAssign(gs);
        }
        else if (TREE_CODE(lhs) == COMPONENT_REF)
        {
          actionData = writeKdmMemberReplace(gs);
        }
        else if (TREE_CODE(lhs) == INDIRECT_REF)
        {
          actionData = writeKdmPtrReplace(gs);
        }
        else if (gimpleRhsCode == ARRAY_REF)
        {
          actionData = writeKdmArraySelect(gs);
        }
        else if (gimpleRhsCode == COMPONENT_REF)
        {
          actionData = writeKdmMemberSelect(gs);
        }
        else if (gimpleRhsCode == INDIRECT_REF)
        {
          actionData = writeKdmPtrSelect(gs);
        }
        else
        {
          boost::format f=boost::format("GIMPLE assignment operation (%1%) in %2% on line (%3%)") % std::string(tree_code_name[gimpleRhsCode]) % BOOST_CURRENT_FUNCTION % __LINE__;
          mKdmWriter.writeUnsupportedComment(boost::str(f));
        }
        break;
      }
      else if (gimpleRhsCode == ADDR_EXPR)
      {
        tree lhs = gimple_assign_lhs(gs);
        if (TREE_CODE(lhs) == COMPONENT_REF)
        {
          //Example: ccf->username = &"nobody"[0];
          actionData = writeKdmMemberReplace(gs);
        }
        else if (TREE_CODE(lhs) == INDIRECT_REF)
        {
          //Example: *a = &b;
          actionData = writeKdmPtrReplace(gs);
        }
        else
        {
          //Example: a = &b;
          actionData = writeKdmPtr(gs);
        }
        break;
      }
      else if (gimpleRhsCode == ADDR_EXPR )
      {
        actionData = writeKdmPtr(gs);
        break;
      }
      else if (gimpleRhsCode == NEGATE_EXPR)
      {
        actionData = writeKdmUnaryOperation(KdmKind::Negate(), gs);
        break;
      }
      else if (gimpleRhsCode == TRUTH_NOT_EXPR)
      {
        actionData = writeKdmUnaryOperation(KdmKind::Not(), gs);
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
          case EXACT_DIV_EXPR:
          case TRUNC_MOD_EXPR:
          case TRUNC_DIV_EXPR:
          case TRUTH_AND_EXPR:
          case TRUTH_OR_EXPR:
          case TRUTH_XOR_EXPR:
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

long GimpleKdmTripleWriter::writeKdmFlow(long const fromId, long const toId)
{
  return writeKdmActionRelation(KdmType::Flow(), fromId, toId);
}

void GimpleKdmTripleWriter::writeKdmUnaryRelationships(long const actionId, long const lhsId, long const rhsId)
{
  writeKdmActionRelation(KdmType::Reads(), actionId, rhsId);
  writeKdmActionRelation(KdmType::Writes(), actionId, lhsId);
}

GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmUnaryConstructor(gimple const gs)
{
  ActionDataPtr actionData = ActionDataPtr(new ActionData(mKdmWriter.getNextElementId()));
  tree lhs(gimple_assign_lhs(gs));
  tree rhs(gimple_assign_rhs1(gs));
  long lhsId = getReferenceId(lhs);
  long tmpId = writeKdmStorableUnit(getReferenceId(TREE_TYPE(rhs)), gimple_location(gs));
  mKdmWriter.writeTripleContains(actionData->actionId(), tmpId);

  actionData->outputId(tmpId);
  mKdmWriter.writeTripleKdmType(actionData->actionId(), KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionData->actionId(), KdmKind::Assign());
  writeKdmUnaryRelationships(actionData->actionId(), lhsId, tmpId);

  return actionData;
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

  writeKdmUnaryRelationships(actionId, lhsId, rhsData->outputId());
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
  writeKdmActionRelation(KdmType::Reads(), actionId, rhsData->outputId());
  writeKdmActionRelation(KdmType::Addresses(), actionId, lhsId);

  //Have to determine if there is a bug in the spec here or not
  //where does the write relationship go?
  mKdmWriter.writeComment("FIXME: KDM spec states there should be a writes relationship here.. ");
  //writeKdmActionRelation(KdmType::Writes(), actionId, lhsId);
  return actionData;
}


GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeBitAssign(gimple const gs)
{
  tree lhs(gimple_assign_lhs(gs));
  tree rhs(gimple_assign_rhs1(gs));
  return writeBitAssign(lhs, rhs, gimple_location(gs));
}

GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeBitAssign(tree const lhs, tree const rhs, location_t const loc)
{
  ActionDataPtr actionData = ActionDataPtr(new ActionData(mKdmWriter.getNextElementId()));
  tree op1 = TREE_OPERAND(rhs, 0);
  long structId = (TREE_CODE(op1) == INDIRECT_REF) ? getReferenceId(TREE_OPERAND(op1, 0)) : getReferenceId(op1);
  long sizeBitId = getReferenceId(TREE_OPERAND(rhs, 1));
  long startBitId = getReferenceId(TREE_OPERAND(rhs, 2));
  long lhsId = (not lhs) ? writeKdmStorableUnit(getReferenceId(TREE_TYPE(rhs)),loc) :  getReferenceId(lhs);

  mKdmWriter.writeTripleKdmType(actionData->actionId(), KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionData->actionId(), KdmKind::BitAssign());
  writeKdmActionRelation(KdmType::Reads(), actionData->actionId(), sizeBitId);
  writeKdmActionRelation(KdmType::Reads(), actionData->actionId(), startBitId);
  writeKdmActionRelation(KdmType::Reads(), actionData->actionId(), structId);
  writeKdmActionRelation(KdmType::Writes(), actionData->actionId(), lhsId);
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
  ActionDataPtr rhs2Data = getRhsReferenceId(rhs2);
  configureDataAndFlow(actionData, rhs1Data, rhs2Data);

  long lhsId = getReferenceId(lhs);
  writeKdmBinaryRelationships(actionId, lhsId, rhs1Data->outputId(), rhs2Data->outputId());
  actionData->outputId(lhsId);
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
  assert(TREE_CODE(rhs) == ARRAY_REF);

  ActionDataPtr arraySelectFlow;

  ActionDataPtr actionData = ActionDataPtr(new ActionData(mKdmWriter.getNextElementId()));
  expanded_location xloc = expand_location(loc);

  tree op0 = TREE_OPERAND (rhs, 0); //var_decl
  tree op1 = TREE_OPERAND (rhs, 1); //index

  ActionDataPtr op0Data = getRhsReferenceId(op0);
  ActionDataPtr op1Data = getRhsReferenceId(op1);

  //Configure Flows and containment
  configureDataAndFlow(actionData, op0Data, op1Data);

  mKdmWriter.writeTripleKdmType(actionData->actionId(), KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionData->actionId(), KdmKind::ArraySelect());
  writeKdmActionRelation(KdmType::Reads(), actionData->actionId(), op1Data->outputId());
  writeKdmActionRelation(KdmType::Addresses(), actionData->actionId(), op0Data->outputId());

  long lhsId = (not lhs) ? writeKdmStorableUnit(getReferenceId(TREE_TYPE(op0)),xloc) : getReferenceId(lhs);

  if (not lhs)
  {
    mKdmWriter.writeTripleContains(actionData->actionId(), lhsId);
  }

  writeKdmActionRelation(KdmType::Writes(), actionData->actionId(), lhsId);
  actionData->outputId(lhsId);
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
  }
  lastId = rhsData->outputId();

  ActionDataPtr selectData;
  if (TREE_CODE(op0) == ARRAY_REF)
  {
    //Multi-dimensional arrays
    while (TREE_CODE(op0) == ARRAY_REF)
    {
      selectData = writeKdmArraySelect(NULL_TREE, op0, gimple_location(gs), false);

      if (!actionData->hasStartAction())
      {
        actionData->startActionId(selectData->actionId());
      }
      else
      {
        writeKdmActionRelation(KdmType::Flow(), lastId ,selectData->actionId());
      }
      mKdmWriter.writeTripleContains(actionId, selectData->actionId());
      lastId = selectData->actionId();

      op1 = TREE_OPERAND (op0, 1);
      op0 = TREE_OPERAND (op0, 0);
    }
  }
  else if (TREE_CODE(op0) == COMPONENT_REF)
  {
    //D.11082->level[1] = D.11083;
    selectData = writeKdmMemberSelect(NULL_TREE, op0, gimple_location(gs));
  }
  else if (TREE_CODE(op0) == VAR_DECL)
  {
    //Do Nothing
  }
  else
  {
    std::string msg(boost::str(boost::format("ArrayReplace type (%1%) in %2%:%3%") % std::string(tree_code_name[TREE_CODE(op0)]) % BOOST_CURRENT_FUNCTION % __LINE__));
    mKdmWriter.writeUnsupportedComment(msg);
  }


  long op0Id = (selectData) ? selectData->outputId() :getReferenceId(op0);
  long op1Id = getReferenceId(op1);

  mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionId, KdmKind::ArrayReplace());
  writeKdmActionRelation(KdmType::Addresses(), actionId, op0Id); //data element
  writeKdmActionRelation(KdmType::Reads(), actionId, op1Id); // index
  writeKdmActionRelation(KdmType::Reads(), actionId, rhsData->outputId()); //new value

  //Have to determine if there is a bug in the spec here or not
  //where does the write relationship go?
  mKdmWriter.writeComment("FIXME: KDM spec states there should be a writes relationship here..");
  actionData->outputId(op0Id);
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
  return writeKdmMemberSelect(lhs, rhs, gimple_location(gs));
}


// Example: a = s.m
//D.1716 = this->m_bar;
//D.4427 = hp->h_length;
GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmMemberSelect(tree const lhs, tree const rhs, location_t const loc)
{
  assert(TREE_CODE(rhs) == COMPONENT_REF);

  expanded_location xloc = expand_location(loc);
  tree op0 = TREE_OPERAND (rhs, 0);
  tree op1 = TREE_OPERAND (rhs, 1);
  ActionDataPtr actionData;
  long lhsId;

  if (TREE_CODE(op0) == INDIRECT_REF)
  {
    //we have to create a temp variable to hold the Ptr result
    tree indirectRef = TREE_OPERAND (op0, 0);
    long refId = getReferenceId(indirectRef);
    lhsId = writeKdmStorableUnit(getReferenceId(TREE_TYPE(indirectRef)),loc);

    //Resolve the indirect reference put result in temp
    ActionDataPtr ptrData = writeKdmPtrSelect(lhsId, refId);

    //Contain the temp variable in the Ptr
    mKdmWriter.writeTripleContains(ptrData->actionId(), lhsId);

    //perform memberselect using temp
    ActionDataPtr op1Data = getRhsReferenceId(op1);
    actionData = writeKdmMemberSelect(lhsId, op1Data->outputId(), ptrData->outputId());
    actionData->startActionId(ptrData->actionId());

    //contain ptr in memberselect
    mKdmWriter.writeTripleContains(actionData->actionId(), ptrData->actionId());

    //Hook up flows and containment if the rhs is an actionElement
    if (op1Data->hasActionId())
    {
      mKdmWriter.writeTripleContains(actionData->actionId(), op1Data->actionId());
      writeKdmFlow(ptrData->actionId(), op1Data->actionId());
      writeKdmFlow(op1Data->actionId(), actionData->actionId());
    }
    //otherwise just flow from the ptr to the memberselect
    else
    {
      writeKdmFlow(ptrData->actionId(), actionData->actionId());
    }
  }

  // Example: a = s.m
  else
  {
    ActionDataPtr lhsData = ActionDataPtr(new ActionData());
    if (not lhs)
    {
      lhsData->outputId(writeKdmStorableUnit(getReferenceId(TREE_TYPE(rhs)),loc));
    }
    else
    {
      lhsData = getRhsReferenceId(lhs);
    }

    ActionDataPtr op0Data = getRhsReferenceId(op0);
    ActionDataPtr op1Data = getRhsReferenceId(op1);
    actionData = writeKdmMemberSelect(lhsData->outputId(), op1Data->outputId(), op0Data->outputId());

    if (lhsData->hasActionId())
    {
      //Shouldn't happen;
      boost::format f = boost::format("MemberSelect has unsupported LHS. %1%") % BOOST_CURRENT_FUNCTION;
      mKdmWriter.writeUnsupportedComment(boost::str(f));

    }
    else
    {
      configureDataAndFlow(actionData, op0Data, op1Data);
    }

    if (not lhs)
    {
      mKdmWriter.writeTripleContains(actionData->actionId(), lhsData->outputId());
    }
  }
  return actionData;
}

GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmMemberSelect(long const writesId, long const readsId, long const addressesId)
{
  long actionId = mKdmWriter.getNextElementId();
  ActionDataPtr actionData(new ActionData(actionId));
  mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionId, KdmKind::MemberSelect());
  writeKdmActionRelation(KdmType::Addresses(), actionId, addressesId);
  writeKdmActionRelation(KdmType::Reads(), actionId, readsId);
  writeKdmActionRelation(KdmType::Writes(), actionId, writesId);
  actionData->outputId(writesId);
  return actionData;
}

void GimpleKdmTripleWriter::configureDataAndFlow(ActionDataPtr actionData, ActionDataPtr op0Data, ActionDataPtr op1Data)
{
  if (op0Data->hasActionId() && op1Data->hasActionId())
  {
    actionData->startActionId(op0Data->actionId());
    writeKdmFlow(op0Data->actionId(), op1Data->actionId());
    writeKdmFlow(op1Data->actionId(), actionData->actionId());
    mKdmWriter.writeTripleContains(actionData->actionId(), op0Data->actionId());
    mKdmWriter.writeTripleContains(actionData->actionId(), op1Data->actionId());
  }
  else if (op0Data->hasActionId())
  {
    actionData->startActionId(op0Data->actionId());
    writeKdmFlow(op0Data->actionId(), actionData->actionId());
    mKdmWriter.writeTripleContains(actionData->actionId(), op0Data->actionId());
  }
  else if (op1Data->hasActionId())
  {
    actionData->startActionId(op1Data->actionId());
    writeKdmFlow(op1Data->actionId(), actionData->actionId());
    mKdmWriter.writeTripleContains(actionData->actionId(), op1Data->actionId());
  }
}

//Example: sin.sin_family = 2;
GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmMemberReplace(gimple const gs)
{
  tree lhs = gimple_assign_lhs(gs);
  tree rhs = gimple_assign_rhs1(gs);
  tree op = TREE_OPERAND (lhs, 0);  //either . or  ->
  return writeKdmMemberReplace(lhs, op, rhs, gimple_location(gs));
}


//Example: sin.sin_family = 2;
GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmMemberReplace(tree const lhs, tree const op, tree const rhs, location_t const loc)
{
  ActionDataPtr actionData = ActionDataPtr(new ActionData(mKdmWriter.getNextElementId()));
  //lhs is a component_ref... we need to get the member to write to
  //which should be second operand if I read my gcc code properly....
  tree member = TREE_OPERAND(lhs, 1);
  long memberId = getReferenceId(member);

  //We just need the reference to the containing struct
  //which should be the first operand....
  ActionDataPtr lhsData = (TREE_CODE(op) == INDIRECT_REF) ? getRhsReferenceId(TREE_OPERAND(op, 0)) : getRhsReferenceId(TREE_OPERAND(lhs, 0));
  ActionDataPtr rhsData = getRhsReferenceId(rhs);
  actionData = writeKdmMemberReplace(memberId, rhsData->outputId(), lhsData->outputId());
  configureDataAndFlow(actionData, lhsData, rhsData);
  return actionData;
}

GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmMemberReplace(long const writesId, long const readsId, long const addressesId)
{
  ActionDataPtr actionData = ActionDataPtr(new ActionData(mKdmWriter.getNextElementId()));
  mKdmWriter.writeTripleKdmType(actionData->actionId(), KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionData->actionId(), KdmKind::MemberReplace());
  writeKdmActionRelation(KdmType::Reads(), actionData->actionId(), readsId);
  writeKdmActionRelation(KdmType::Addresses(), actionData->actionId(), addressesId);
  writeKdmActionRelation(KdmType::Writes(), actionData->actionId(), writesId);
  return actionData;
}


//  //Example: ptr = &a[0];
GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmPtr(gimple const gs)
{
  tree lhs = gimple_assign_lhs(gs);
  tree rhs = gimple_assign_rhs1(gs);
  return writeKdmPtr(lhs, rhs, gimple_location(gs));
}


GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmPtr(tree const lhs, tree const rhs, location_t const loc)
{
  ActionDataPtr actionData(new ActionData());

  tree op0 = TREE_OPERAND (rhs, 0);
  long lhsId;

  //  //Example: ptr = &a[0];
  if (TREE_CODE(op0) == ARRAY_REF)
  {
    //Perform arraySelect
    ActionDataPtr selectData = writeKdmArraySelect(NULL_TREE, op0, loc, false);

    //Determine lhsid
    lhsId = (not lhs) ? writeKdmStorableUnit(getReferenceId(TREE_TYPE(TREE_OPERAND (op0, 0))),loc) : getReferenceId(lhs);

    //Write Ptr Element
    actionData = writeKdmPtr(lhsId, selectData->outputId());
    actionData->startActionId(*selectData);

    if (not lhs)
    {
      mKdmWriter.writeTripleContains(actionData->actionId(), lhsId);
    }

    //Contain the arrayselect in the ptr
    mKdmWriter.writeTripleContains(actionData->actionId(), selectData->actionId());

    //Hook up flow from select to ptr
    writeKdmFlow(selectData->actionId(),actionData->actionId());
  }
  else if (TREE_CODE(op0) == COMPONENT_REF)
  {
    // a = &b.d;
    actionData = writeKdmMemberSelect(NULL_TREE, op0, loc);
  }
  //  // Example: a = &b;
  else
  {
    //we have to create a temp variable... aka temp1
    long storableId = writeKdmStorableUnit(getReferenceId(TREE_TYPE(op0)),gcckdm::locationOf(rhs));

    //Address.... temp1 = &rhs
    ActionDataPtr rhsData = getRhsReferenceId(op0);
    actionData = writeKdmPtr(storableId, rhsData->outputId());

    //Contain the tmp in the ptr action element
    mKdmWriter.writeTripleContains(actionData->actionId(), storableId);

    //Hook up flows if required
    if (rhsData->hasActionId())
    {
      actionData->startActionId(*rhsData);
      writeKdmFlow(rhsData->actionId(), actionData->actionId());
      mKdmWriter.writeTripleContains(actionData->actionId(), rhsData->actionId());
    }
  }
  return actionData;
}




GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmPtr(long const writesId, long const addressesId)
{
  long actionId = mKdmWriter.getNextElementId();
  ActionDataPtr actionData(new ActionData(actionId));
  mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionId, KdmKind::Ptr());
  writeKdmActionRelation(KdmType::Addresses(), actionId, addressesId);
  writeKdmActionRelation(KdmType::Writes(), actionId, writesId);
  actionData->outputId(writesId);
  return actionData;
}


GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmPtrSelect(gimple const gs)
{
  tree lhs = gimple_assign_lhs(gs);
  tree rhs = gimple_assign_rhs1(gs);
  return writeKdmPtrSelect(lhs, rhs, gimple_location(gs));

}

GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmPtrSelect(tree const lhs, tree const rhs, location_t const loc)
{
  assert(TREE_CODE(rhs) == INDIRECT_REF);

  tree op0 = TREE_OPERAND (rhs, 0);
  ActionDataPtr actionData = ActionDataPtr(new ActionData());
  ActionDataPtr lhsData;
  if (not lhs)
  {
    lhsData = ActionDataPtr(new ActionData());
    lhsData->outputId(writeKdmStorableUnit(getReferenceId(TREE_TYPE(op0)),loc));
  }
  else
  {
    lhsData = getRhsReferenceId(lhs);
  }

  ActionDataPtr rhsData = getRhsReferenceId(op0);
  actionData = writeKdmPtrSelect(lhsData->outputId(), rhsData->outputId());
  //Contain the tmp in the ptr action element
  if (not lhs)
  {
    mKdmWriter.writeTripleContains(actionData->actionId(), lhsData->outputId());
  }
  configureDataAndFlow(actionData, lhsData, rhsData);

  return actionData;
}

//GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmPtrSelect(long const writesId, long const readsId, long const addressesId)
GimpleKdmTripleWriter::ActionDataPtr GimpleKdmTripleWriter::writeKdmPtrSelect(long const writesId, long const addressesId)
{
  ActionDataPtr actionData(new ActionData(mKdmWriter.getNextElementId()));
  mKdmWriter.writeTripleKdmType(actionData->actionId(), KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionData->actionId(), KdmKind::PtrSelect());
  writeKdmActionRelation(KdmType::Writes(), actionData->actionId(), writesId);
  //FIXME: We skip this reads for the momment need to clarification in the KDM Spec
  //writeKdmActionRelation(KdmType::Reads(), actionId, readsId);
  writeKdmActionRelation(KdmType::Addresses(), actionData->actionId(), addressesId);
  actionData->outputId(writesId);
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
