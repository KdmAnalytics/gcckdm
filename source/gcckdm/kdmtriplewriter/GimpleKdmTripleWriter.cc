/*
 * KdmTripleGimpleWriter.cc
 *
 *  Created on: Jul 13, 2010
 *      Author: kgirard
 */

#include "gcckdm/kdmtriplewriter/GimpleKdmTripleWriter.hh"
#include "gcckdm/kdmtriplewriter/KdmTripleWriter.hh"
#include "gcckdm/KdmKind.hh"
#include "boost/current_function.hpp"

namespace
{

void gimple_not_implemented_yet(gimple const gs)
{
    std::cerr << "Unknown GIMPLE statement: " << gimple_code_name[static_cast<int> (gimple_code(gs))] << std::endl;
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
            CASE_CONVERT: rhsString += "(" + gcckdm::getAstNodeName(TREE_TYPE(lhs)) + ") ";
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
            if (TREE_CODE_CLASS(rhs_code) == tcc_declaration || TREE_CODE_CLASS(rhs_code) == tcc_constant || TREE_CODE_CLASS(rhs_code)
                    == tcc_reference || rhs_code == SSA_NAME || rhs_code == ADDR_EXPR || rhs_code == CONSTRUCTOR)
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
            rhsString += " <" + gcckdm::getAstNodeName(gimple_assign_rhs1(gs)) + ", " + gcckdm::getAstNodeName(gimple_assign_rhs2(gs))
                    + ">";
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
                rhsString += gcckdm::getAstNodeName(gimple_assign_rhs1(gs)) + " " + std::string(op_symbol_code(gimple_assign_rhs_code(gs)))
                        + " ";
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
    std::cerr << "TernaryRhsString not implemented" << std::endl;
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

    std::cerr << "================GIMPLE START==========================\n";
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
                std::cerr << "Gimple statement not handled yet" << std::endl;
                break;
            }

        }

    }
    std::cerr << "================GIMPLE END==========================\n";

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

    long blockId = getBlockReferenceId(gimple_location(gs));
    mKdmWriter.writeTripleContains(blockId, actionId);
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
            rhsString += "=====Gimple Operation Not Implemented========1";
            break;

        case FIXED_CONVERT_EXPR:
        case ADDR_SPACE_CONVERT_EXPR:
        case FIX_TRUNC_EXPR:
        case FLOAT_EXPR:
            CASE_CONVERT: rhsString += "=====Gimple Operation Not Implemented========2";
            //
            //            rhsString += "(" + gcckdm::getAstNodeName(TREE_TYPE(lhs)) + ") ";
            //            if (op_prio(rhs) < op_code_prio(rhs_code))
            //            {
            //                rhsString += "(" + gcckdm::getAstNodeName(rhs) + ")";
            //            }
            //            else
            //                rhsString += gcckdm::getAstNodeName(rhs);
            break;

        case PAREN_EXPR:
            rhsString += "=====Gimple Operation Not Implemented========4";
            //            rhsString += "((" + gcckdm::getAstNodeName(rhs) + "))";
            break;

        case ABS_EXPR:
            rhsString += "=====Gimple Operation Not Implemented========5";
            //            rhsString += "ABS_EXPR <" + gcckdm::getAstNodeName(rhs) + ">";
            break;

        default:
            if (TREE_CODE_CLASS(rhs_code) == tcc_declaration || TREE_CODE_CLASS(rhs_code) == tcc_constant || TREE_CODE_CLASS(rhs_code)
                    == tcc_reference || rhs_code == SSA_NAME || rhs_code == ADDR_EXPR || rhs_code == CONSTRUCTOR)
            {
                writeKdmUnaryOperation(actionId, KdmKind::Assign(), gs);
                break;
            }
            else if (rhs_code == BIT_NOT_EXPR)
            {
                rhsString += "=====Gimple Operation Not Implemented========7";
                //                rhsString += '~';
            }
            else if (rhs_code == TRUTH_NOT_EXPR)
            {
                rhsString += "=====Gimple Operation Not Implemented========8";
                //                rhsString += '!';
            }
            else if (rhs_code == NEGATE_EXPR)
            {
                rhsString += "=====Gimple Operation Not Implemented========9";
                //                rhsString += "-";
            }
            else
            {
                rhsString += "=====Gimple Operation Not Implemented========10";
                //                rhsString += "[" + std::string(tree_code_name[rhs_code]) + "]";
            }

            if (op_prio(rhs) < op_code_prio(rhs_code))
            {
                rhsString += "=====Gimple Operation Not Implemented========11";
                //                rhsString += "(" + gcckdm::getAstNodeName(rhs) + ")";
            }
            else
            {
                rhsString += "=====Gimple Operation Not Implemented========12";
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
                rhsString += gcckdm::getAstNodeName(gimple_assign_rhs1(gs)) + " " + std::string(op_symbol_code(gimple_assign_rhs_code(gs)))
                        + " ";

                switch(gimple_assign_rhs_code(gs))
                {
                    case PLUS_EXPR:
                    {
                        writeKdmBinaryOperation(actionId, KdmKind::Add(), gs);
                        break;
                    }
                    default:
                    {
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
    long rhsId = getRhsReferenceId(actionId,rhs);
    writeKdmUnaryRelationships(actionId, lhsId, rhsId);

}

void GimpleKdmTripleWriter::writeKdmBinaryOperation(long const actionId, KdmKind const & kind, gimple const gs)
{
    tree lhs = gimple_assign_lhs(gs);
    tree rhs1 = gimple_assign_rhs1(gs);
    tree rhs2 = gimple_assign_rhs2(gs);

    mKdmWriter.writeTripleKind(actionId, kind);
    long lhsId = mKdmWriter.getReferenceId(lhs);
    long rhs1Id = getRhsReferenceId(actionId,rhs1);
    long rhs2Id = getRhsReferenceId(actionId,rhs2);
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

    //If this node isn't referenced we haven't seen this value before and it should be
    //processed and contained within the ActionElement
    if (!mKdmWriter.hasReferenceId(rhs))
    {
        rhsId = mKdmWriter.getReferenceId(rhs);
        mKdmWriter.processAstNode(rhs);
        mKdmWriter.writeTripleContains(actionId, rhsId);
    }
    // If this node is referenced we have to check to see it's type before
    // we say it's contained in the action element.
    else
    {
        rhsId = mKdmWriter.getReferenceId(rhs);
        if (TREE_CODE(rhs) == INTEGER_CST)
        {
            mKdmWriter.writeTripleContains(actionId, rhsId);
        }
//        else
//        {
//            std::cerr << BOOST_CURRENT_FUNCTION << ": Unsupported RHS :" << tree_code_name[TREE_CODE(rhs)] << std::endl;
//        }
    }
    return rhsId;
}


} // namespace kdmtriplewriter


} // namespace gcckdm

