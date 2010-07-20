/*
 * KdmTripleGimpleWriter.cc
 *
 *  Created on: Jul 13, 2010
 *      Author: kgirard
 */

#include "gcckdm/kdmtriplewriter/GimpleKdmTripleWriter.hh"
#include "gcckdm/kdmtriplewriter/KdmTripleWriter.hh"
#include "gcckdm/KdmKind.hh"

namespace
{

bool isValueNode(tree const node)
{
  return TREE_CODE(node) == INTEGER_CST || TREE_CODE(node) == REAL_CST;
}


void gimple_not_implemented_yet(gimple const gs)
{
  std::cerr << "# UNSUPPORTED: GIMPLE statement: " << gimple_code_name[static_cast<int> (gimple_code(gs))] << std::endl;
  std::cerr << "# UNSUPPORTED: ";
  print_gimple_stmt(stderr, gs, 0, 0);
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
  mKdmWriter(tripleWriter)
{
}

GimpleKdmTripleWriter::~GimpleKdmTripleWriter()
{

}

void GimpleKdmTripleWriter::processGimpleSequence(tree const parent, gimple_seq const seq)
{
  for (gimple_stmt_iterator i = gsi_start(seq); !gsi_end_p(i); gsi_next(&i))
  {
    gimple gs = gsi_stmt(i);
    processGimpleStatement(parent, gs);
  }
}

void GimpleKdmTripleWriter::processGimpleStatement(tree const parent, gimple const gs)
{

  mKdmWriter.writeComment("================GIMPLE START==========================");
  if (gs)
  {
    switch (gimple_code(gs))
    {
      case GIMPLE_ASM:
      {
        gimple_not_implemented_yet(gs);
        break;
      }
      case GIMPLE_ASSIGN:
      {
        //gimple_not_implemented_yet(gs);
        processGimpleAssignStatement(parent, gs);
        break;
      }
      case GIMPLE_BIND:
      {
        processGimpleBindStatement(parent, gs);
        //debug_gimple_stmt(gs);
        break;
      }
      case GIMPLE_CALL:
      {
        gimple_not_implemented_yet(gs);
        break;
      }
      case GIMPLE_COND:
      {
        gimple_not_implemented_yet(gs);
        break;
      }
      case GIMPLE_LABEL:
      {
        gimple_not_implemented_yet(gs);
        break;
      }
      case GIMPLE_GOTO:
      {
        gimple_not_implemented_yet(gs);
        break;
      }
      case GIMPLE_NOP:
      {
        gimple_not_implemented_yet(gs);
        break;
      }
      case GIMPLE_RETURN:
      {
        gimple_not_implemented_yet(gs);
//        processGimpleBindStatement(parent, gs);
        break;
      }
      case GIMPLE_SWITCH:
      {
        gimple_not_implemented_yet(gs);
        break;
      }
      case GIMPLE_TRY:
      {
        gimple_not_implemented_yet(gs);
        break;
      }
      case GIMPLE_PHI:
      {
        gimple_not_implemented_yet(gs);
        break;
      }
      case GIMPLE_OMP_PARALLEL:
      {
        gimple_not_implemented_yet(gs);
        break;
      }
      case GIMPLE_OMP_TASK:
      {
        gimple_not_implemented_yet(gs);
        break;
      }
      case GIMPLE_OMP_ATOMIC_LOAD:
      {
        gimple_not_implemented_yet(gs);
        break;
      }
      case GIMPLE_OMP_ATOMIC_STORE:
      {
        gimple_not_implemented_yet(gs);
        break;
      }
      case GIMPLE_OMP_FOR:
      {
        gimple_not_implemented_yet(gs);
        break;
      }
      case GIMPLE_OMP_CONTINUE:
      {
        gimple_not_implemented_yet(gs);
        break;
      }
      case GIMPLE_OMP_SINGLE:
      {
        gimple_not_implemented_yet(gs);
        break;
      }
      case GIMPLE_OMP_RETURN:
      {
        gimple_not_implemented_yet(gs);
        break;
      }
      case GIMPLE_OMP_SECTIONS:
      {
        gimple_not_implemented_yet(gs);
        break;
      }
      case GIMPLE_OMP_SECTIONS_SWITCH:
      {
        gimple_not_implemented_yet(gs);
        break;
      }
      case GIMPLE_OMP_MASTER:
      case GIMPLE_OMP_ORDERED:
      case GIMPLE_OMP_SECTION:
      {
        gimple_not_implemented_yet(gs);
        break;
      }
      case GIMPLE_OMP_CRITICAL:
      {
        gimple_not_implemented_yet(gs);
        break;
      }
      case GIMPLE_CATCH:
      {
        gimple_not_implemented_yet(gs);
        break;
      }
      case GIMPLE_EH_FILTER:
      {
        gimple_not_implemented_yet(gs);
        break;
      }
      case GIMPLE_EH_MUST_NOT_THROW:
      {
        gimple_not_implemented_yet(gs);
        break;
      }
      case GIMPLE_RESX:
      {
        gimple_not_implemented_yet(gs);
        break;
      }
      case GIMPLE_EH_DISPATCH:
      {
        gimple_not_implemented_yet(gs);
        break;
      }
      case GIMPLE_DEBUG:
      {
        gimple_not_implemented_yet(gs);
        break;
      }
      case GIMPLE_PREDICT:
      {
        gimple_not_implemented_yet(gs);
        break;
      }
      default:
      {
        mKdmWriter.writeComment("UNHANDLED: Gimple statement");
        break;
      }

    }

  }
  mKdmWriter.writeComment("================GIMPLE END==========================");

}

void GimpleKdmTripleWriter::processGimpleBindStatement(tree const parent, gimple const gs)
{
  tree var;
  for (var = gimple_bind_vars(gs); var; var = TREE_CHAIN(var))
  {
    long declId = mKdmWriter.getReferenceId(var);
    mKdmWriter.processAstNode(var);
    mKdmWriter.writeTripleContains(mKdmWriter.getReferenceId(parent), declId);
  }

  processGimpleSequence(parent, gimple_bind_body(gs));
}

void GimpleKdmTripleWriter::processGimpleAssignStatement(tree const parent, gimple const gs)
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

  long blockId = getBlockReferenceId(gimple_location(gs));
  mKdmWriter.writeTripleContains(blockId, actionId);
}

void GimpleKdmTripleWriter::processGimpleReturnStatement(tree const parent, gimple const gs)
{
  gimple_not_implemented_yet(gs);
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
      mKdmWriter.writeComment("=====Gimple Operation Not Implemented========1");
      break;

    case FIXED_CONVERT_EXPR:
    case ADDR_SPACE_CONVERT_EXPR:
    case FIX_TRUNC_EXPR:
    case FLOAT_EXPR:
    CASE_CONVERT
    :
      mKdmWriter.writeComment("FIXME: This Assign is really a cast, but we do not support casts");
      writeKdmUnaryOperation(actionId, KdmKind::Assign(), gs);

      break;

    case PAREN_EXPR:
      mKdmWriter.writeComment("=====Gimple Operation Not Implemented========3");
      //            rhsString += "((" + gcckdm::getAstNodeName(rhs) + "))";
      break;

    case ABS_EXPR:
      mKdmWriter.writeComment("=====Gimple Operation Not Implemented========4");
      //            rhsString += "ABS_EXPR <" + gcckdm::getAstNodeName(rhs) + ">";
      break;

    default:
      if (TREE_CODE_CLASS(rhs_code) == tcc_declaration || TREE_CODE_CLASS(rhs_code) == tcc_constant || TREE_CODE_CLASS(rhs_code) == tcc_reference
          || rhs_code == SSA_NAME || rhs_code == ADDR_EXPR || rhs_code == CONSTRUCTOR)
      {
        writeKdmUnaryOperation(actionId, KdmKind::Assign(), gs);
        break;
      }
      else
      {
        mKdmWriter.writeComment("Gimple Operation Not Implemented:" + std::string(tree_code_name[rhs_code]));
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
      //      else if (rhs_code == NEGATE_EXPR)
      //      {
      //        mKdmWriter.writeComment("=====Gimple Operation Not Implemented========9");
      //        //                rhsString += "-";
      //      }
      //      else
      //      {
      //        rhsString += "=====Gimple Operation Not Implemented========10";
      //        //                rhsString += "[" + std::string(tree_code_name[rhs_code]) + "]";
      //      }

      if (op_prio(rhs) < op_code_prio(rhs_code))
      {
        mKdmWriter.writeComment("Gimple Operation Not Implemented:" + std::string(tree_code_name[rhs_code]));
        //        rhsString += "=====Gimple Operation Not Implemented========11";
        //                rhsString += "(" + gcckdm::getAstNodeName(rhs) + ")";
      }
      else
      {
        mKdmWriter.writeComment("Gimple Operation Not Implemented:" + std::string(tree_code_name[rhs_code]));
        //        rhsString += "=====Gimple Operation Not Implemented========12";
        //                rhsString += gcckdm::getAstNodeName(rhs);
      }
      break;
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
          {
            writeKdmBinaryOperation(actionId, KdmKind::Add(), gs);
            break;
          }
          case MINUS_EXPR:
          {
            writeKdmBinaryOperation(actionId, KdmKind::Subtract(), gs);
            break;
          }
          case MULT_EXPR:
          {
            writeKdmBinaryOperation(actionId, KdmKind::Multiply(), gs);
            break;
          }
          case TRUNC_DIV_EXPR:
          case RDIV_EXPR:
          case EXACT_DIV_EXPR:
          {
            writeKdmBinaryOperation(actionId, KdmKind::Divide(), gs);
            break;
          }
          default:
          {
            mKdmWriter.writeComment(std::string("Unsupported rhs op_code: ") + op_symbol_code(gimple_assign_rhs_code(gs)));
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
  std::cerr << "rhsbinaryString: " << rhsString << std::endl;

}

void GimpleKdmTripleWriter::processGimpleTernaryAssignStatement(long const actionId, gimple const gs)
{
  std::cerr << "=====Gimple Operation Not Implemented======== -> processGimpleTernaryAssignStatement" << std::endl;
}

long GimpleKdmTripleWriter::getBlockReferenceId(location_t const loc)
{
  expanded_location xloc = expand_location(loc);
  LocationMap::iterator i = mBlockUnitMap.find(xloc);
  long blockId;
  if (i == mBlockUnitMap.end())
  {
    blockId = mKdmWriter.getNextElementId();
    mBlockUnitMap.insert(std::make_pair(xloc, blockId));
    mKdmWriter.writeTripleKdmType(blockId, KdmType::BlockUnit());
    mKdmWriter.writeKdmSourceRef(blockId, xloc);
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

