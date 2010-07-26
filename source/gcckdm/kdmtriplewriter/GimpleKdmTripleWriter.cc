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
                              (TRUTH_OR_EXPR, gcckdm::KdmKind::Or())
                              (TRUTH_XOR_EXPR, gcckdm::KdmKind::Xor())
                              ;

bool isValueNode(tree const node)
{
  return TREE_CODE(node) == INTEGER_CST || TREE_CODE(node) == REAL_CST;
}

std::string getUnaryRhsString(gimple const gs)
{
  std::string rhsString;

  enum tree_code rhs_code = gimple_assign_rhs_code(gs);
  tree lhs = gimple_assign_lhs(gs);
  tree rhs = gimple_assign_rhs1(gs);

  switch (rhs_code)
  {
    case VIEW_CONVERT_EXPR:
    case ASSERT_EXPR:
      rhsString += gcckdm::getAstNodeName(rhs);
      break;

    case FIXED_CONVERT_EXPR:
    case ADDR_SPACE_CONVERT_EXPR:
    case FIX_TRUNC_EXPR:
    case FLOAT_EXPR:
    CASE_CONVERT
    :
      rhsString += "(" + gcckdm::getAstNodeName(TREE_TYPE(lhs)) + ") ";
      if (op_prio(rhs) < op_code_prio(rhs_code))
      {
        rhsString += "(" + gcckdm::getAstNodeName(rhs) + ")";
      }
      else
        rhsString += gcckdm::getAstNodeName(rhs);
      break;

    case PAREN_EXPR:
      rhsString += "((" + gcckdm::getAstNodeName(rhs) + "))";
      break;

    case ABS_EXPR:
      rhsString += "ABS_EXPR <" + gcckdm::getAstNodeName(rhs) + ">";
      break;

    default:
      if (TREE_CODE_CLASS(rhs_code) == tcc_declaration || TREE_CODE_CLASS(rhs_code) == tcc_constant || TREE_CODE_CLASS(rhs_code) == tcc_reference
          || rhs_code == SSA_NAME || rhs_code == ADDR_EXPR || rhs_code == CONSTRUCTOR)
      {
        rhsString += gcckdm::getAstNodeName(rhs);
        break;
      }
      else if (rhs_code == BIT_NOT_EXPR)
      {
        rhsString += '~';
      }
      else if (rhs_code == TRUTH_NOT_EXPR)
      {
        rhsString += '!';
      }
      else if (rhs_code == NEGATE_EXPR)
      {
        rhsString += "-";
      }
      else
      {
        rhsString += "[" + std::string(tree_code_name[rhs_code]) + "]";
      }

      if (op_prio(rhs) < op_code_prio(rhs_code))
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
  mKdmWriter(tripleWriter), mLabelFlag(false), mLastLabelId(0)
{
}

GimpleKdmTripleWriter::~GimpleKdmTripleWriter()
{

}

void GimpleKdmTripleWriter::processGimpleSequence(tree const parent, gimple_seq const seq)
{
  mKdmWriter.writeComment("================GIMPLE START SEQUENCE " + gcckdm::getAstNodeName(parent) + "==========================");
  for (gimple_stmt_iterator i = gsi_start(seq); !gsi_end_p(i); gsi_next(&i))
  {
    gimple gs = gsi_stmt(i);
    processGimpleStatement(parent, gs);
  }
  mKdmWriter.writeComment("================GIMPLE END SEQUENCE " + gcckdm::getAstNodeName(parent) + "==========================");
}

void GimpleKdmTripleWriter::processGimpleStatement(tree const parent, gimple const gs)
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
        actionId = processGimpleAssignStatement(parent, gs);
        break;
      }
      case GIMPLE_BIND:
      {
        processGimpleBindStatement(parent, gs);
        hasActionId = false;
        break;
      }
      case GIMPLE_CALL:
      {
        processGimpleCallStatement(parent, gs);
        break;
      }
      case GIMPLE_COND:
      {
        actionId = processGimpleConditionalStatement(parent, gs);
        break;
      }
      case GIMPLE_LABEL:
      {
        processGimpleLabelStatement(parent, gs);
        hasActionId = false;
        break;
      }
        //      case GIMPLE_GOTO:
        //      {
        //        gimple_not_implemented_yet(gs);
        //        break;
        //      }
        //      case GIMPLE_NOP:
        //      {
        //        gimple_not_implemented_yet(gs);
        //        break;
        //      }
      case GIMPLE_RETURN:
      {
        actionId = processGimpleReturnStatement(parent, gs);
        break;
      }
        //              case GIMPLE_SWITCH:
        //      {
        //        gimple_not_implemented_yet(gs);
        //        break;
        //      }
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
    //If the last gimple statement we processed was a label
    // we have to do a little magic here to get the flows
    // correct... labels don't have line numbers so we add
    // the label to the next actionElement after the label
    if (mLabelFlag and hasActionId)
    {
      long blockId = getBlockReferenceId(parent, gimple_location(gs));
      mKdmWriter.writeTripleContains(blockId, mLastLabelId);
      mLabelFlag = !mLabelFlag;
    }
  }

}

void GimpleKdmTripleWriter::processGimpleBindStatement(tree const parent, gimple const gs)
{
  mKdmWriter.writeComment("================GIMPLE START BIND STATEMENT " + gcckdm::getAstNodeName(parent) + "==========================");
  tree var;
  for (var = gimple_bind_vars(gs); var; var = TREE_CHAIN(var))
  {
    long declId = mKdmWriter.getReferenceId(var);
    mKdmWriter.processAstNode(var);
    mKdmWriter.writeTripleContains(mKdmWriter.getReferenceId(parent), declId);
  }
  processGimpleSequence(parent, gimple_bind_body(gs));
  mKdmWriter.writeComment("================GIMPLE END BIND STATEMENT " + gcckdm::getAstNodeName(parent) + "==========================");
}

long GimpleKdmTripleWriter::processGimpleAssignStatement(tree const parent, gimple const gs)
{
  long actionId = mKdmWriter.getNextElementId();
  mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
  unsigned numOps(gimple_num_ops(gs));
  if (numOps == 2)
  {
    processGimpleUnaryAssignStatement(actionId, gs);
  }
  else if (numOps == 3)
  {
    processGimpleBinaryAssignStatement(actionId, gs);
  }
  else if (numOps == 4)
  {
    processGimpleTernaryAssignStatement(actionId, gs);
  }
  else
  {
    mKdmWriter.writeComment("GimpleKdmTripleWriter::processGimpleAssignStatement: Unsupported number of operations");
  }

  long blockId = getBlockReferenceId(parent, gimple_location(gs));
  mKdmWriter.writeTripleContains(blockId, actionId);
  return actionId;
}

long GimpleKdmTripleWriter::processGimpleReturnStatement(tree const parent, gimple const gs)
{
  long actionId = mKdmWriter.getNextElementId();
  mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionId, KdmKind::Return());
  tree t = gimple_return_retval(gs);
  long id = mKdmWriter.getReferenceId(t);
  mKdmWriter.processAstNode(t);
  writeKdmActionRelation(KdmType::Reads(), actionId, id);

  //figure out what blockunit this statement belongs
  long blockId = getBlockReferenceId(parent, gimple_location(gs));
  mKdmWriter.writeTripleContains(blockId, actionId);
  return actionId;
}

long GimpleKdmTripleWriter::processGimpleLabelStatement(tree const parent, gimple const gs)
{
  tree label = gimple_label_label(gs);

  long actionId;
  //  if (mKdmWriter.hasReferenceId(label))
  //  {
  actionId = writeKdmNopForLabel(label);
  mLastLabelId = actionId;
  mLabelFlag = true;
  //  }
  //  else
  //  {
  //    actionId = writeKdmNopForLabel(label);
  //  }
  return actionId;
  //mKdmWriter.processAstNode(label);
}

long GimpleKdmTripleWriter::writeKdmNopForLabel(tree const label)
{
  long actionId = mKdmWriter.getReferenceId(label);
  mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionId, KdmKind::Nop());
  mKdmWriter.writeTripleName(actionId, gcckdm::getAstNodeName(label));
  return actionId;
}

long GimpleKdmTripleWriter::processGimpleConditionalStatement(tree const parent, gimple const gs)
{
  long actionId = mKdmWriter.getNextElementId();
  mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionId, KdmKind::Condition());
  mKdmWriter.writeTripleName(actionId, "if");

  if (gimple_cond_true_label(gs))
  {
    tree trueNode(gimple_cond_true_label(gs));
    long trueFlowId(mKdmWriter.getNextElementId());
    long trueNodeId(mKdmWriter.getReferenceId(trueNode));

    mKdmWriter.writeTripleKdmType(trueFlowId, KdmType::TrueFlow());
    mKdmWriter.writeTriple(trueFlowId, KdmPredicate::From(), actionId);
    mKdmWriter.writeTriple(trueFlowId, KdmPredicate::To(), trueNodeId);
    mKdmWriter.writeTripleContains(actionId, trueFlowId);
  }
  if (gimple_cond_false_label(gs))
  {
    tree falseNode(gimple_cond_false_label(gs));
    long falseFlowId(mKdmWriter.getNextElementId());
    long falseNodeId(mKdmWriter.getReferenceId(falseNode));
    mKdmWriter.writeTripleKdmType(falseFlowId, KdmType::FalseFlow());
    mKdmWriter.writeTriple(falseFlowId, KdmPredicate::From(), actionId);
    mKdmWriter.writeTriple(falseFlowId, KdmPredicate::To(), falseNodeId);
    mKdmWriter.writeTripleContains(actionId, falseFlowId);
  }
  return actionId;
}

long GimpleKdmTripleWriter::processGimpleCallStatement(tree const parent, gimple const gs)
{
  long actionId = mKdmWriter.getNextElementId();
  mKdmWriter.writeTripleKdmType(actionId, KdmType::ActionElement());
  mKdmWriter.writeTripleKind(actionId, KdmKind::Call());

  long callId = mKdmWriter.getNextElementId();
  mKdmWriter.writeTripleKdmType(actionId, KdmType::Call());
  mKdmWriter.writeTriple(callId, KdmPredicate::From(), actionId);

  tree op0 = gimple_call_fn(gs);
  if (TREE_CODE (op0) == NON_LVALUE_EXPR)
  {
    op0 = TREE_OPERAND (op0, 0);
  }
  tree t(resolveCall(op0));
  long callableId(mKdmWriter.getReferenceId(t));
  mKdmWriter.writeTriple(callId, KdmPredicate::To(), callableId);
  mKdmWriter.writeTripleContains(actionId, callId);

  //Read each parameter
  if (gimple_call_num_args(gs) > 0)
  {
    for (size_t i = 0; i < gimple_call_num_args(gs); i++)
    {
      long paramId(mKdmWriter.getReferenceId(gimple_call_arg(gs, i)));
      writeKdmActionRelation(KdmType::Reads(), actionId, paramId);
    }
  }

  //optional write
  tree lhs = gimple_call_lhs(gs);
  if (lhs)
  {
    long lhsId(mKdmWriter.getReferenceId(lhs));
    writeKdmActionRelation(KdmType::Writes(), actionId, lhsId);
  }

  return actionId;
}

tree GimpleKdmTripleWriter::resolveCall(tree const node)
{
  tree op0 = node;
  tree_code code = TREE_CODE (op0);
  switch (code)
  {
    case FUNCTION_DECL:
    {
      break;
    }
    case ADDR_EXPR:
    {
      op0 = TREE_OPERAND(op0, 0);
      resolveCall(op0);
      break;
    }
    default:
    {
      std::string msg(boost::str(boost::format("GIMPLE call statement (%1%) in %2%") % std::string(tree_code_name[TREE_CODE(op0)])
          % BOOST_CURRENT_FUNCTION));
      mKdmWriter.writeUnsupportedComment(msg);
    }
      break;
  }
  return op0;
}

void GimpleKdmTripleWriter::processGimpleUnaryAssignStatement(long const actionId, gimple const gs)
{
  std::string rhsString;
  tree rhs = gimple_assign_rhs1(gs);

  enum tree_code rhs_code = gimple_assign_rhs_code(gs);
  switch (rhs_code)
  {
    case VIEW_CONVERT_EXPR:
    case ASSERT_EXPR:
    {
      std::string msg(boost::str(boost::format("GIMPLE assignment statement (%1%) in %2%") % std::string(tree_code_name[rhs_code])
          % BOOST_CURRENT_FUNCTION));
      mKdmWriter.writeUnsupportedComment(msg);
      break;
    }
    case FIXED_CONVERT_EXPR:
    case ADDR_SPACE_CONVERT_EXPR:
    case FIX_TRUNC_EXPR:
    case FLOAT_EXPR:
    CASE_CONVERT
    :
    {
      mKdmWriter.writeComment("FIXME: This Assign is really a cast, but we do not support casts");
      writeKdmUnaryOperation(actionId, KdmKind::Assign(), gs);

      break;
    }
    case PAREN_EXPR:
    {
      std::string msg(boost::str(boost::format("GIMPLE assignment operation (%1%) in %2%") % std::string(tree_code_name[rhs_code])
          % BOOST_CURRENT_FUNCTION));
      mKdmWriter.writeUnsupportedComment(msg);
      //            rhsString += "((" + gcckdm::getAstNodeName(rhs) + "))";
      break;
    }
    case ABS_EXPR:
    {
      std::string msg(boost::str(boost::format("GIMPLE assignment operation (%1%) in %2%") % std::string(tree_code_name[rhs_code])
          % BOOST_CURRENT_FUNCTION));
      mKdmWriter.writeUnsupportedComment(msg);
      //            rhsString += "ABS_EXPR <" + gcckdm::getAstNodeName(rhs) + ">";
      break;
    }
    default:
    {
      if (TREE_CODE_CLASS(rhs_code) == tcc_declaration || TREE_CODE_CLASS(rhs_code) == tcc_constant || TREE_CODE_CLASS(rhs_code) == tcc_reference
          || rhs_code == SSA_NAME || rhs_code == ADDR_EXPR || rhs_code == CONSTRUCTOR)
      {
        writeKdmUnaryOperation(actionId, KdmKind::Assign(), gs);
        break;
      }
      else if (rhs_code == NEGATE_EXPR)
      {
        writeKdmUnaryOperation(actionId, KdmKind::Negate(), gs);
        break;
      }
      else
      {
        std::string msg(boost::str(boost::format("GIMPLE assignment operation (%1%) in %2% on line (%3%)") % std::string(tree_code_name[rhs_code])
            % BOOST_CURRENT_FUNCTION % __LINE__));
        mKdmWriter.writeUnsupportedComment(msg);
      }
      //      else if (rhs_code == BIT_NOT_EXPR)
      //      {
      //        mKdmWriter.writeComment("=====Gimple Operation Not Implemented========7");
      //        //                rhsString += '~';
      //      }
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

      if (op_prio(rhs) < op_code_prio(rhs_code))
      {
        std::string msg(boost::str(boost::format("GIMPLE assignment operation (%1%) in %2% on line (%3%)") % std::string(tree_code_name[rhs_code])
            % BOOST_CURRENT_FUNCTION % __LINE__));
        mKdmWriter.writeUnsupportedComment(msg);
        //        rhsString += "=====Gimple Operation Not Implemented========11";
        //                rhsString += "(" + gcckdm::getAstNodeName(rhs) + ")";
      }
      else
      {
        std::string msg(boost::str(boost::format("GIMPLE assignment operation (%1%) in %2% on line (%3%)") % std::string(tree_code_name[rhs_code])
            % BOOST_CURRENT_FUNCTION % __LINE__));
        mKdmWriter.writeUnsupportedComment(msg);
        //        rhsString += "=====Gimple Operation Not Implemented========12";
        //                rhsString += gcckdm::getAstNodeName(rhs);
      }
      break;
    }
  }
}

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
          case MINUS_EXPR:
          case MULT_EXPR:
          case RDIV_EXPR:
          case LT_EXPR:
          case LE_EXPR:
          case GT_EXPR:
          case GE_EXPR:
          case EQ_EXPR:
          case NE_EXPR:
         {
            writeKdmBinaryOperation(actionId, treeCodeToKind.find(gimple_assign_rhs_code(gs))->second, gs);
            break;
          }
          default:
          {
            mKdmWriter.writeUnsupportedComment(std::string("rhs op_code: (") + op_symbol_code(gimple_assign_rhs_code(gs)) + ") in " + BOOST_CURRENT_FUNCTION );
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

long GimpleKdmTripleWriter::getBlockReferenceId(tree const parent, location_t const loc)
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
    mKdmWriter.writeTripleContains(mKdmWriter.getReferenceId(parent), blockId);
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
  long lhsId = mKdmWriter.getReferenceId(lhs);
  long rhsId = getRhsReferenceId(actionId, rhs);
  writeKdmUnaryRelationships(actionId, lhsId, rhsId);

}

void GimpleKdmTripleWriter::writeKdmBinaryOperation(long const actionId, KdmKind const & kind, gimple const gs)
{
  tree lhs = gimple_assign_lhs(gs);
  tree rhs1 = gimple_assign_rhs1(gs);
  tree rhs2 = gimple_assign_rhs2(gs);

  mKdmWriter.writeTripleKind(actionId, kind);
  long lhsId = mKdmWriter.getReferenceId(lhs);
  long rhs1Id = getRhsReferenceId(actionId, rhs1);
  long rhs2Id = getRhsReferenceId(actionId, rhs2);
  writeKdmBinaryRelationships(actionId, lhsId, rhs1Id, rhs2Id);

}

void GimpleKdmTripleWriter::writeKdmBinaryRelationships(long const actionId, long const lhsId, long const rhs1Id, long const rhs2Id)
{
  writeKdmActionRelation(KdmType::Writes(), actionId, lhsId);
  writeKdmActionRelation(KdmType::Reads(), actionId, rhs1Id);
  writeKdmActionRelation(KdmType::Reads(), actionId, rhs2Id);
}

long GimpleKdmTripleWriter::getRhsReferenceId(long const actionId, tree const rhs)
{
  long rhsId;

  //Value types we handle special have to add them to languageUnit
  if (isValueNode(rhs))
  {
    rhsId = mKdmWriter.getReferenceId(rhs);
    mKdmWriter.processAstNode(rhs);
  }
  else
  {
    rhsId = mKdmWriter.getReferenceId(rhs);
  }
  return rhsId;
}

} // namespace kdmtriplewriter


} // namespace gcckdm

