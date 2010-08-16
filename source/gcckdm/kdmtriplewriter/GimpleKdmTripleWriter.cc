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
    case ASSERT_EXPR:
      rhsString += gcckdm::getAstNodeName(rhs);
      break;

    case FIXED_CONVERT_EXPR:
    case ADDR_SPACE_CONVERT_EXPR:
    case FIX_TRUNC_EXPR:
    case FLOAT_EXPR:
    case NOP_EXPR:
    case CONVERT_EXPR:
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

    case PAREN_EXPR:
      rhsString += "((" + gcckdm::getAstNodeName(rhs) + "))";
      break;

    case ABS_EXPR:
      rhsString += "ABS_EXPR <" + gcckdm::getAstNodeName(rhs) + ">";
      break;

    default:
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
  return rhsString;
}

std::string getBinaryRhsString(gimple const gs)
{
  std::string rhsString("");
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
  //    ///Might not need this function I don't know
  //
  //    std::string rhsString();
  //    enum tree_code code = gimple_assign_rhs_code (gs);
  //    switch (code)
  //      {
  //      case WIDEN_MULT_PLUS_EXPR:
  //      case WIDEN_MULT_MINUS_EXPR:
  //      {
  //          rhsString += tree_code_name [static_cast<int>(code)];
  //          std::transform(rhsString.begin(), rhsString.end(), rhsString.begin(), toupper);
  //          rhsString += " <" + gcckdm::getAstNodeName(gimple_assign_rhs1(gs)) + ", " + gcckdm::getAstNodeName(gimple_assign_rhs2(gs)) + ", " + gcckdm::getAstNodeName(gimple_assign_rhs3(gs)) + ">";
  //        break;
  //      }
  //
  //      default:
  //      {
  //        gcc_unreachable ();
  //      }

}

} // namespace

namespace gcckdm
{

namespace kdmtriplewriter
{


GimpleKdmTripleWriter::GimpleKdmTripleWriter(KdmTripleWriter & tripleWriter) :
      mKdmWriter(tripleWriter),
      mLabelFlag(false),
      mLastLabelId(0),
      mRegisterVariableIndex(0),
      mLastActionId(0),
      mHasLastActionId(false)
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
    mCurrentFunctionDeclarationNode = functionDeclNode;
    mCurrentCallableUnitId = getReferenceId(mCurrentFunctionDeclarationNode);

    //
    mHasLastActionId = false;

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

long GimpleKdmTripleWriter::getRhsReferenceId(tree const rhs)
{
  long rhsId;

  //Value types we handle special have to add them to languageUnit
  if (isValueNode(rhs))
  {
    rhsId = getReferenceId(rhs);
    mKdmWriter.processAstNode(rhs);
  }
  else
  {
    rhsId = getReferenceId(rhs);
  }
  return rhsId;
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
  long actionId;
  bool hasActionId(true);
  if (gs)
  {
    switch (gimple_code(gs))
    {
      //      case GIMPLE_ASM:
      //      {
      //        gimple_not_implemented_yet(mKdmWriter, gs);
      //        break;
      //      }
      case GIMPLE_ASSIGN:
      {
        actionId = processGimpleAssignStatement(gs);
        break;
      }
      case GIMPLE_BIND:
      {
        processGimpleBindStatement(gs);
        hasActionId = false;
        break;
      }
      case GIMPLE_CALL:
      {
        processGimpleCallStatement(gs);
        break;
      }
      case GIMPLE_COND:
      {
        actionId = processGimpleConditionalStatement(gs);
        break;
      }
      case GIMPLE_LABEL:
      {
        processGimpleLabelStatement(gs);
        hasActionId = false;
        break;
      }
      case GIMPLE_GOTO:
      {
        processGimpleGotoStatement(gs);
        hasActionId = false;
        break;
      }
      //      case GIMPLE_NOP:
      //      {
      //        gimple_not_implemented_yet(gs);
      //        break;
      //      }
      case GIMPLE_RETURN:
      {
        actionId = processGimpleReturnStatement(gs);
        break;
      }
      case GIMPLE_SWITCH:
      {
        processGimpleSwitchStatement(gs);
        hasActionId = false;
        break;
      }
      //      case GIMPLE_TRY:
      //      {
      //        gimple_not_implemented_yet(gs);
      //        break;
      //      }
      //      case GIMPLE_PHI:
      //      {
      //        gimple_not_implemented_yet(gs);
      //        break;
      //      }
      //      case GIMPLE_OMP_PARALLEL:
      //      {
      //        gimple_not_implemented_yet(gs);
      //        break;
      //      }
      //      case GIMPLE_OMP_TASK:
      //      {
      //        gimple_not_implemented_yet(gs);
      //        break;
      //      }
      //      case GIMPLE_OMP_ATOMIC_LOAD:
      //      {
      //        gimple_not_implemented_yet(gs);
      //        break;
      //      }
      //      case GIMPLE_OMP_ATOMIC_STORE:
      //      {
      //        gimple_not_implemented_yet(gs);
      //        break;
      //      }
      //      case GIMPLE_OMP_FOR:
      //      {
      //        gimple_not_implemented_yet(gs);
      //        break;
      //      }
      //      case GIMPLE_OMP_CONTINUE:
      //      {
      //        gimple_not_implemented_yet(gs);
      //        break;
      //      }
      //      case GIMPLE_OMP_SINGLE:
      //      {
      //        gimple_not_implemented_yet(gs);
      //        break;
      //      }
      //      case GIMPLE_OMP_RETURN:
      //      {
      //        gimple_not_implemented_yet(gs);
      //        break;
      //      }
      //      case GIMPLE_OMP_SECTIONS:
      //      {
      //        gimple_not_implemented_yet(gs);
      //        break;
      //      }
      //      case GIMPLE_OMP_SECTIONS_SWITCH:
      //      {
      //        gimple_not_implemented_yet(gs);
      //        break;
      //      }
      //      case GIMPLE_OMP_MASTER:
      //      case GIMPLE_OMP_ORDERED:
      //      case GIMPLE_OMP_SECTION:
      //      {
      //        gimple_not_implemented_yet(gs);
      //        break;
      //      }
      //      case GIMPLE_OMP_CRITICAL:
      //      {
      //        gimple_not_implemented_yet(gs);
      //        break;
      //      }
      //      case GIMPLE_CATCH:
      //      {
      //        gimple_not_implemented_yet(gs);
      //        break;
      //      }
      //      case GIMPLE_EH_FILTER:
      //      {
      //        gimple_not_implemented_yet(gs);
      //        break;
      //      }
      //      case GIMPLE_EH_MUST_NOT_THROW:
      //      {
      //        gimple_not_implemented_yet(gs);
      //        break;
      //      }
      //      case GIMPLE_RESX:
      //      {
      //        gimple_not_implemented_yet(gs);
      //        break;
      //      }
      //      case GIMPLE_EH_DISPATCH:
      //      {
      //        gimple_not_implemented_yet(gs);
      //        break;
      //      }
      //      case GIMPLE_DEBUG:
      //      {
      //        gimple_not_implemented_yet(gs);
      //        break;
      //      }
      //      case GIMPLE_PREDICT:
      //      {
      //        gimple_not_implemented_yet(gs);
      //        break;
      //      }
      default:
      {
        std::string msg(boost::str(boost::format("GIMPLE statement (%1%) in %2%") % gimple_code_name[static_cast<int> (gimple_code(gs))]
                                                                                                     % BOOST_CURRENT_FUNCTION));
        mKdmWriter.writeUnsupportedComment(msg);
        break;
      }

    }

    //After the first action element we need to hook up the flows
    if (mHasLastActionId)
    {
      long flowId = mKdmWriter.getNextElementId();
      writeKdmActionRelation(KdmType::Flow(), mLastActionId, flowId);
    }

    //the gimple we just processed returned an action remember it
    if (hasActionId)
    {
      mLastActionId = actionId;
      mHasLastActionId = true;
    }


    // If the last gimple statement we processed was a label or some goto's
    // we have to do a little magic here to get the flows
    // correct... some labels/goto's don't have line numbers so we add
    // the label to the next actionElement after the label
    // mLabelFlag is set in the processGimpleLabelStatement and processGimpleGotoStatement methods
    //
    if (mLabelFlag and hasActionId)
    {
      long blockId = getBlockReferenceId(gimple_location(gs));
      mKdmWriter.writeTripleContains(blockId, mLastLabelId);
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

long GimpleKdmTripleWriter::processGimpleAssignStatement(gimple const gs)
{
  long actionElementId = mKdmWriter.getNextElementId();
  mKdmWriter.writeTripleKdmType(actionElementId, KdmType::ActionElement());

  unsigned numOps(gimple_num_ops(gs));
  if (numOps == 2)
  {
    processGimpleUnaryAssignStatement(actionElementId, gs);
  }
  else if (numOps == 3)
  {
    processGimpleBinaryAssignStatement(actionElementId, gs);
  }
  else if (numOps == 4)
  {
    processGimpleTernaryAssignStatement(actionElementId, gs);
  }
  else
  {
    mKdmWriter.writeComment("GimpleKdmTripleWriter::processGimpleAssignStatement: Unsupported number of operations");
  }

  //This action element is contained in a block unit
  long blockId = getBlockReferenceId(gimple_location(gs));
  mKdmWriter.writeTripleContains(blockId, actionElementId);
  return actionElementId;
}

long GimpleKdmTripleWriter::processGimpleReturnStatement(gimple const gs)
{
  long actionId = mKdmWriter.getNextElementId();
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
  return actionId;
}

long GimpleKdmTripleWriter::processGimpleLabelStatement(gimple const gs)
{
  tree label = gimple_label_label(gs);

  long actionId;
  actionId = writeKdmNopForLabel(label);
  mLastLabelId = actionId;
  mLabelFlag = true;
  return actionId;
}

long GimpleKdmTripleWriter::writeKdmNopForLabel(tree const label)
{
  long actionId = getReferenceId(label);
  mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionId, KdmKind::Nop());
  mKdmWriter.writeTripleName(actionId, gcckdm::getAstNodeName(label));
  return actionId;
}

long GimpleKdmTripleWriter::processGimpleConditionalStatement(gimple const gs)
{
  long actionId = mKdmWriter.getNextElementId();
  mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionId, KdmKind::Condition());
  mKdmWriter.writeTripleName(actionId, "if");

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

  return actionId;
}

long GimpleKdmTripleWriter::processGimpleCallStatement(gimple const gs)
{
  long actionId = mKdmWriter.getNextElementId();
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
      long paramId;
      //parameter to a function is taking the address of something
      if (TREE_CODE(callArg) == ADDR_EXPR)
      {
        paramId = writeKdmPtrParam(actionId, t, callArg, gs);
      }
      else if (TREE_CODE(callArg) == COMPONENT_REF)
      {
        paramId = writeKdmMemberSelectParam(actionId, callArg, gs);
      }
      else
      {
        paramId = (getReferenceId(callArg));
      }

      writeKdmActionRelation(KdmType::Reads(), actionId, paramId);
    }
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


  return actionId;
}


long GimpleKdmTripleWriter::processGimpleGotoStatement(gimple const gs)
{
  long actionId = mKdmWriter.getNextElementId();
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
    mLastLabelId = actionId;
    mLabelFlag = true;
  }
  else
  {
    long blockId = getBlockReferenceId(gimple_location(gs));
    mKdmWriter.writeTripleContains(blockId, actionId);
  }
  return actionId;
}


void GimpleKdmTripleWriter::processGimpleUnaryAssignStatement(long const actionElementId, gimple const gs)
{
  std::string rhsString;
  tree rhsNode = gimple_assign_rhs1(gs);

  enum tree_code gimpleRhsCode = gimple_assign_rhs_code(gs);
  switch (gimpleRhsCode)
  {
    case VIEW_CONVERT_EXPR:
    case ASSERT_EXPR:
    {
      std::string msg(boost::str(boost::format("GIMPLE assignment statement (%1%) in %2%") % std::string(tree_code_name[gimpleRhsCode])
      % BOOST_CURRENT_FUNCTION));
      mKdmWriter.writeUnsupportedComment(msg);
      break;
    }
    case FIXED_CONVERT_EXPR:
    case ADDR_SPACE_CONVERT_EXPR:
    case FIX_TRUNC_EXPR:
    case FLOAT_EXPR:
    case NOP_EXPR:
    case CONVERT_EXPR:
    {
      //Casting a pointer
      if (TREE_CODE(rhsNode) == ADDR_EXPR)
      {
        writeKdmPtr(actionElementId, gs);
        break;
      }
      else
      {
        //Simple cast, no pointers or references
        mKdmWriter.writeComment("FIXME: This Assign is really a cast, but we do not support casts");
        writeKdmUnaryOperation(actionElementId, KdmKind::Assign(), gs);
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
          writeKdmPtrReplace(actionElementId, gs);
        }
        else if (TREE_CODE(lhs) == COMPONENT_REF)
        {
          writeKdmMemberReplace(actionElementId, gs);
        }
        else if (TREE_CODE(lhs) == ARRAY_REF)
        {
          writeKdmArrayReplace(actionElementId, gs);
        }
        else
        {
          writeKdmUnaryOperation(actionElementId, KdmKind::Assign(), gs);
        }
        break;
      }
      else if (TREE_CODE_CLASS(gimpleRhsCode) == tcc_reference)
      {
        if (gimpleRhsCode == ARRAY_REF)
        {
          writeKdmArraySelect(actionElementId, gs);
        }
        else if(gimpleRhsCode == COMPONENT_REF)
        {
          writeKdmMemberSelect(actionElementId, gs);
          break;
        }
        else if (gimpleRhsCode == INDIRECT_REF)
        {
          writeKdmPtr(actionElementId, gs);
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
        writeKdmPtr(actionElementId, gs);
        break;
      }
      else if (gimpleRhsCode == NEGATE_EXPR)
      {
        writeKdmUnaryOperation(actionElementId, KdmKind::Negate(), gs);
        break;
      }
      else if (gimpleRhsCode == BIT_NOT_EXPR)
      {
        writeKdmUnaryOperation(actionElementId, KdmKind::BitNot(), gs);
        break;
      }
      else
      {
        std::string msg(boost::str(boost::format("GIMPLE assignment operation (%1%) in %2% on line (%3%)") % std::string(tree_code_name[gimpleRhsCode])  % BOOST_CURRENT_FUNCTION % __LINE__));
        mKdmWriter.writeUnsupportedComment(msg);
        break;
      }
      //      else if (rhs_code == TRUTH_NOT_EXPR)
      //      {
      //        mKdmWriter.writeComment("=====Gimple Operation Not Implemented========8");
      //        //                rhsString += '!';
      //      }
      //            else
      //      {
      //        rhsString += "=====Gimple Operation Not Implemented========10";
      //        //                rhsString += "[" + std::string(tree_code_name[rhs_code]) + "]";
      //      }

//      if (op_prio(rhsNode) < op_code_prio(gimpleRhsCode))
//      {
//        std::string msg(boost::str(boost::format("GIMPLE assignment operation (%1%) in %2% on line (%3%)") % std::string(tree_code_name[gimpleRhsCode])
//        % BOOST_CURRENT_FUNCTION % __LINE__));
//        mKdmWriter.writeUnsupportedComment(msg);
//      }
//      else
//      {
//        std::string msg(boost::str(boost::format("GIMPLE assignment operation (%1%) in %2% on line (%3%)") % std::string(tree_code_name[gimpleRhsCode])
//        % BOOST_CURRENT_FUNCTION % __LINE__));
//        mKdmWriter.writeUnsupportedComment(msg);
//      }
      break;
    }
  }
}


//long GimpleKdmTripleWriter::processGimpleAstNode(long const actionId, gimple const gs, tree const ast)
//{
//  switch (TREE_CODE(ast))
//  {
//    case ADDR_EXPR:
//    {
//      writeKdmPtr(actionId, gs);
//      break;
//    }
//    case ARRAY_REF:
//    {
//      break;
//    }
//    case INDIRECT_REF:
//    {
//      break;
//    }
//    default:
//    {
//      std::string msg(boost::str(boost::format("GIMPLE AST Node (%1%) in %2% on line (%3%)") % std::string(tree_code_name[ast])
//      % BOOST_CURRENT_FUNCTION % __LINE__));
//
//    }
//  }
//  return 0;
//}


void GimpleKdmTripleWriter::processGimpleBinaryAssignStatement(long const actionId, gimple const gs)
{
  std::string rhsString;
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
            writeKdmBinaryOperation(actionId, treeCodeToKind.find(gimple_assign_rhs_code(gs))->second, gs);
            break;
          }
          default:
          {
            std::string msg(boost::str(boost::format("GIMPLE binary assignment operation '%1%' (%2%) in %3% on line %4%") % op_symbol_code(gimple_assign_rhs_code(gs)) % std::string(tree_code_name[gimple_assign_rhs_code(gs)]) % BOOST_CURRENT_FUNCTION % __LINE__));
            mKdmWriter.writeUnsupportedComment(msg);
//            std::string(tree_code_name[gimpleRhsCode])
//            mKdmWriter.writeUnsupportedComment(std::string("rhs op_code: (") + op_symbol_code(gimple_assign_rhs_code(gs)) + ") in " + BOOST_CURRENT_FUNCTION );
            break;
          }
        }
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
  //std::cerr << "rhsbinaryString: " << rhsString << std::endl;

}

void GimpleKdmTripleWriter::processGimpleTernaryAssignStatement(long const actionId, gimple const gs)
{
  enum tree_code rhs_code = gimple_assign_rhs_code(gs);
  std::string msg(boost::str(boost::format("GIMPLE assignment operation (%1%) in %2%") % std::string(tree_code_name[rhs_code])
  % BOOST_CURRENT_FUNCTION));
  mKdmWriter.writeUnsupportedComment(msg);
}


void GimpleKdmTripleWriter::processGimpleSwitchStatement(gimple const gs)
{
  long actionId = mKdmWriter.getNextElementId();
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

      long gurardedFlowId(mKdmWriter.getNextElementId());
      mKdmWriter.writeTripleKdmType(gurardedFlowId, KdmType::GuardedFlow());
      mKdmWriter.writeTriple(gurardedFlowId, KdmPredicate::From(), gurardedFlowId);
      mKdmWriter.writeTriple(gurardedFlowId, KdmPredicate::To(), readsId);
      mKdmWriter.writeTripleContains(actionId, gurardedFlowId);
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
  writeKdmActionRelation(KdmType::Writes(), actionId, lhsId);
  writeKdmActionRelation(KdmType::Reads(), actionId, rhsId);
}

void GimpleKdmTripleWriter::writeKdmUnaryOperation(long const actionId, KdmKind const & kind, gimple const gs)
{
  tree lhs = gimple_assign_lhs(gs);
  tree rhs = gimple_assign_rhs1(gs);

  mKdmWriter.writeTripleKind(actionId, kind);
  long lhsId = getReferenceId(lhs);
  long rhsId = getRhsReferenceId(rhs);
  writeKdmUnaryRelationships(actionId, lhsId, rhsId);

}

void GimpleKdmTripleWriter::writeKdmPtrReplace(long const actionId, gimple const gs)
{
  tree lhs = gimple_assign_lhs(gs);
  tree rhs = gimple_assign_rhs1(gs);

  tree lhsOp0 = TREE_OPERAND (lhs, 0);
  mKdmWriter.writeTripleKind(actionId, KdmKind::PtrReplace());
  long lhsId = getReferenceId(lhsOp0);
  long rhsId = getRhsReferenceId(rhs);

  writeKdmActionRelation(KdmType::Reads(), actionId, rhsId);
  writeKdmActionRelation(KdmType::Addresses(), actionId, lhsId);

  //Have to determine if there is a bug in the spec here or not
  //where does the write relationship go?
  mKdmWriter.writeComment("FIXME: KDM spec states there should be a writes relationship here.. ");
  //writeKdmActionRelation(KdmType::Writes(), actionId, lhsId);


}

void GimpleKdmTripleWriter::writeKdmBinaryOperation(long const actionId, KdmKind const & kind, gimple const gs)
{
  tree lhs = gimple_assign_lhs(gs);
  tree rhs1 = gimple_assign_rhs1(gs);
  tree rhs2 = gimple_assign_rhs2(gs);

  mKdmWriter.writeTripleKind(actionId, kind);
  long lhsId = getReferenceId(lhs);
  long rhs1Id = getRhsReferenceId(rhs1);
  long rhs2Id = getRhsReferenceId(rhs2);
  writeKdmBinaryRelationships(actionId, lhsId, rhs1Id, rhs2Id);
}

//D.1989 = a[23];
void GimpleKdmTripleWriter::writeKdmArraySelect(long const actionElementId, gimple const gs)
{
  tree lhs = gimple_assign_lhs(gs);
  tree rhs = gimple_assign_rhs1(gs);
  tree op0 = TREE_OPERAND (rhs, 0);
  tree op1 = TREE_OPERAND (rhs, 1);

  mKdmWriter.writeTripleKind(actionElementId, KdmKind::ArraySelect());
  long lhsId = getReferenceId(lhs);
  long op0Id = getRhsReferenceId(op0);
  long op1Id = getRhsReferenceId(op1);

  writeKdmActionRelation(KdmType::Reads(), actionElementId, op1Id);
  writeKdmActionRelation(KdmType::Addresses(), actionElementId, op0Id);
  writeKdmActionRelation(KdmType::Writes(), actionElementId, lhsId);
}

// foo[0] = 1
void GimpleKdmTripleWriter::writeKdmArrayReplace(long const actionElementId, gimple const gs)
{
  tree lhs = gimple_assign_lhs(gs);
  tree rhs = gimple_assign_rhs1(gs);

  tree op0 = TREE_OPERAND (lhs, 0);
  tree op1 = TREE_OPERAND (lhs, 1);

  mKdmWriter.writeTripleKind(actionElementId, KdmKind::ArrayReplace());
  long rhsId = getRhsReferenceId(rhs);
  long op0Id = getReferenceId(op0);
  long op1Id = getReferenceId(op1);

  writeKdmActionRelation(KdmType::Addresses(), actionElementId, op0Id); //data element
  writeKdmActionRelation(KdmType::Reads(), actionElementId, op1Id); // index
  writeKdmActionRelation(KdmType::Reads(), actionElementId, rhsId); //new value

  //Have to determine if there is a bug in the spec here or not
  //where does the write relationship go?
  mKdmWriter.writeComment("FIXME: KDM spec states there should be a writes relationship here..");
}

/** Write to LHS
 * Addresses object
 * Reads member
 *
 */
//D.1716 = this->m_bar;
//D.4427 = hp->h_length;
void GimpleKdmTripleWriter::writeKdmMemberSelect(long const actionElementId, gimple const gs)
{
  tree lhs = gimple_assign_lhs(gs);
  tree rhs = gimple_assign_rhs1(gs);
  tree op0 = TREE_OPERAND (rhs, 0);
  tree op1 = TREE_OPERAND (rhs, 1);


  if (TREE_CODE(op0) == INDIRECT_REF)
  {
    tree indirectRef = TREE_OPERAND (op0, 0);
    long refId = getReferenceId(indirectRef);

    //we have to create a temp variable to hold the Ptr result
    long storableId = writeKdmStorableUnit(getReferenceId(TREE_TYPE(indirectRef)),expand_location(gimple_location(gs)));
    long ptrActionId = mKdmWriter.getNextElementId();
    mKdmWriter.writeTripleContains(ptrActionId, storableId);

    //Resolve the indirect reference put result in temp
    mKdmWriter.writeTripleKdmType(ptrActionId, KdmType::ActionElement());
    mKdmWriter.writeTripleKind(ptrActionId, KdmKind::Ptr());
    writeKdmActionRelation(KdmType::Addresses(), ptrActionId, refId);
    writeKdmActionRelation(KdmType::Writes(), ptrActionId, storableId);

    //perform memberselect using temp
    long op1Id = getRhsReferenceId(op1);
    long lhsId = getReferenceId(lhs);
    mKdmWriter.writeTripleKind(actionElementId, KdmKind::MemberSelect());
    writeKdmActionRelation(KdmType::Reads(), actionElementId, op1Id);
    writeKdmActionRelation(KdmType::Invokes(), actionElementId, ptrActionId);
    writeKdmActionRelation(KdmType::Writes(), actionElementId, lhsId);
  }
  else
  {
    mKdmWriter.writeTripleKind(actionElementId, KdmKind::MemberSelect());
    long lhsId = getReferenceId(lhs);
    long op0Id = getRhsReferenceId(op0);
    long op1Id = getRhsReferenceId(op1);

    writeKdmActionRelation(KdmType::Reads(), actionElementId, op1Id);
    writeKdmActionRelation(KdmType::Invokes(), actionElementId, op0Id);
    writeKdmActionRelation(KdmType::Writes(), actionElementId, lhsId);
  }
}



long GimpleKdmTripleWriter::writeKdmMemberSelectParam(long const actionElementId, tree const compRef, gimple const gs)
{
  const long blockUnitId(getBlockReferenceId(gimple_location(gs)));
  tree op0 = TREE_OPERAND (compRef, 0);
  tree op1 = TREE_OPERAND (compRef, 1);

  mKdmWriter.writeTripleKind(actionElementId, KdmKind::MemberSelect());

  //we have to create a temp variable
  long storableId = writeKdmStorableUnit(getReferenceId(TREE_TYPE(op0)), expand_location(gimple_location(gs)));
  mKdmWriter.writeTripleContains(blockUnitId, storableId);

  long op0Id = getRhsReferenceId(op0);
  long op1Id = getRhsReferenceId(op1);

  writeKdmActionRelation(KdmType::Reads(), actionElementId, op1Id);
  writeKdmActionRelation(KdmType::Invokes(), actionElementId, op0Id);
  writeKdmActionRelation(KdmType::Writes(), actionElementId, storableId);
  return storableId;
}


//Example: sin.sin_family = 2;
void GimpleKdmTripleWriter::writeKdmMemberReplace(long const actionElementId, gimple const gs)
{
  tree lhs = gimple_assign_lhs(gs);
  tree rhs = gimple_assign_rhs1(gs);

  tree lhsOp0 = TREE_OPERAND (lhs, 0);
  mKdmWriter.writeTripleKind(actionElementId, KdmKind::MemberReplace());
  long lhsId = getReferenceId(lhsOp0);
  long rhsId = getRhsReferenceId(rhs);

  writeKdmActionRelation(KdmType::Reads(), actionElementId, rhsId);
  writeKdmActionRelation(KdmType::Invokes(), actionElementId, lhsId);
  writeKdmActionRelation(KdmType::Writes(), actionElementId, lhsId);
}




//ptr = &a[0];
void GimpleKdmTripleWriter::writeKdmPtr(long const actionElementId, gimple const gs)
{
  const long blockUnitId(getBlockReferenceId(gimple_location(gs)));
  tree lhs = gimple_assign_lhs(gs);

  //this should be an addr_expr
  tree rhs = gimple_assign_rhs1(gs);

  tree op0 = TREE_OPERAND (rhs, 0);

  if (TREE_CODE(op0) == ARRAY_REF)
  {
    tree refOp0 = TREE_OPERAND (op0, 0);
    tree refOp1 = TREE_OPERAND (op0, 1);
    long refOp0Id = getRhsReferenceId(refOp0);
    long refOp1Id = getRhsReferenceId(refOp1);


    //we have to create a temp variable
    long storableId = writeKdmStorableUnit(getReferenceId(TREE_TYPE(refOp0)),expand_location(gimple_location(gs)));

    long ptrActionId = mKdmWriter.getNextElementId();
    mKdmWriter.writeTripleContains(blockUnitId, storableId);

    mKdmWriter.writeTripleKind(actionElementId, KdmKind::ArraySelect());
    writeKdmActionRelation(KdmType::Reads(), actionElementId, refOp1Id);
    writeKdmActionRelation(KdmType::Addresses(), actionElementId, refOp0Id);
    writeKdmActionRelation(KdmType::Writes(), actionElementId, storableId);

    mKdmWriter.writeTripleKdmType(ptrActionId, KdmType::ActionElement());
    mKdmWriter.writeTripleKind(ptrActionId, KdmKind::Ptr());
    writeKdmActionRelation(KdmType::Addresses(), ptrActionId, storableId);
    tree lhs = gimple_assign_lhs(gs);
    long lhsId = getReferenceId(lhs);
    writeKdmActionRelation(KdmType::Writes(), ptrActionId, lhsId);
  }
  else
  {
    long lhsId = getReferenceId(lhs);
    long rhsId = getRhsReferenceId(op0);
    mKdmWriter.writeTripleKind(actionElementId, KdmKind::Ptr());
    writeKdmActionRelation(KdmType::Addresses(), actionElementId, rhsId);
    writeKdmActionRelation(KdmType::Writes(), actionElementId, lhsId);
  }
}

long GimpleKdmTripleWriter::writeKdmPtrParam(long const callActionId, tree const callNode, tree const addrExpr, gimple const gs)
{
  //All of these elements are contained in the same block unit
  const long blockUnitId(getBlockReferenceId(gimple_location(gs)));
  tree op0 = TREE_OPERAND (addrExpr, 0);
  long tempId;

  if (TREE_CODE(op0) == ARRAY_REF)
  {
    tree refOp0 = TREE_OPERAND (op0, 0);
    tree refOp1 = TREE_OPERAND (op0, 1);
    long refOp0Id = getRhsReferenceId(refOp0);
    long refOp1Id = getRhsReferenceId(refOp1);

    long ptrActionId = mKdmWriter.getNextElementId();
    long arraySelectActionId = mKdmWriter.getNextElementId();

    //we have to create a temp variable... aka temp1
    long storableId = writeKdmStorableUnit(getReferenceId(TREE_TYPE(TREE_OPERAND (op0, 0))),expand_location(gcckdm::locationOf(addrExpr)));
    mKdmWriter.writeTripleContains(blockUnitId, storableId);

    //ArraySelect ... ie  temp1 = myArray[0]
    mKdmWriter.writeTripleKind(arraySelectActionId, KdmKind::ArraySelect());
    writeKdmActionRelation(KdmType::Reads(), arraySelectActionId, refOp1Id);
    writeKdmActionRelation(KdmType::Addresses(), arraySelectActionId, refOp0Id);
    writeKdmActionRelation(KdmType::Writes(), arraySelectActionId, storableId);
    mKdmWriter.writeTripleContains(blockUnitId, arraySelectActionId);

    //we have to create a second temp variable... aka temp2
    long storable2Id = writeKdmStorableUnit(getReferenceId(TREE_TYPE(TREE_OPERAND (op0, 0))),expand_location(gcckdm::locationOf(addrExpr)));
    mKdmWriter.writeTripleContains(blockUnitId, storable2Id);

    //perform address on temp1 assign to temp2    temp2 = &rhs
    mKdmWriter.writeTripleKdmType(ptrActionId, KdmType::ActionElement());
    mKdmWriter.writeTripleKind(ptrActionId, KdmKind::Ptr());
    writeKdmActionRelation(KdmType::Addresses(), ptrActionId, storableId);
    writeKdmActionRelation(KdmType::Writes(), callActionId, storable2Id);
    mKdmWriter.writeTripleContains(blockUnitId, ptrActionId);

    tempId = storable2Id;
  }
  else if (TREE_CODE(op0) == COMPONENT_REF)
  {
    long selectId = mKdmWriter.getNextElementId();
    mKdmWriter.writeTripleKdmType(selectId, KdmType::ActionElement());
    tempId = writeKdmMemberSelectParam(selectId, op0, gs);
  }
  else
  {
    //we have to create a temp variable... aka temp1
    long storableId = writeKdmStorableUnit(getReferenceId(TREE_TYPE(TREE_OPERAND (addrExpr, 0))),expand_location(gcckdm::locationOf(addrExpr)));
    mKdmWriter.writeTripleContains(blockUnitId, storableId);

    //Address.... temp1 = &rhs
    long ptrActionId = mKdmWriter.getNextElementId();

    long rhsId = getRhsReferenceId(op0);
    mKdmWriter.writeTripleKdmType(ptrActionId, KdmType::ActionElement());
    mKdmWriter.writeTripleKind(ptrActionId, KdmKind::Ptr());
    writeKdmActionRelation(KdmType::Addresses(), ptrActionId, rhsId);
    writeKdmActionRelation(KdmType::Writes(), ptrActionId, storableId);
    mKdmWriter.writeTripleContains(blockUnitId, rhsId);
    tempId = storableId;
  }

  return tempId;
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
  writeKdmActionRelation(KdmType::Writes(), actionId, lhsId);
  writeKdmActionRelation(KdmType::Reads(), actionId, rhs1Id);
  writeKdmActionRelation(KdmType::Reads(), actionId, rhs2Id);
}


} // namespace kdmtriplewriter


} // namespace gcckdm

