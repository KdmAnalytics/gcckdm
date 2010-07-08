/*
 * KdmTripleWriter.cc
 *
 *  Created on: Jun 21, 2010
 *      Author: kgirard
 */

#include "gcckdm/kdmtriplewriter/KdmTripleWriter.hh"

#include <algorithm>
#include <iterator>

#include "boost/filesystem/fstream.hpp"
#include "boost/filesystem/operations.hpp"
#include "boost/lexical_cast.hpp"

#include "gcckdm/GccKdmConfig.hh"
#include "gcckdm/KdmPredicate.hh"

namespace
{
//If a AST node doesn't have a name use this name
std::string const unamedNode("<unnamed>");

/**
 * Returns the name of the given node or the value of unnamedNode
 *
 * @param node the node to query for it's name
 *
 * @return the name of the given node or the value of unnamedNode
 */
std::string nodeName(tree const node)
{
    return gcckdm::getAstNodeName(node);
}

/**
 * True if the given AST node is an annoymous struct
 *
 * @param t the node to test
 * @return true if t is an annoymous struct
 */
bool isAnonymousStruct(tree const t)
{
    tree name = TYPE_NAME (t);
    if (name && TREE_CODE (name) == TYPE_DECL)
        name = DECL_NAME (name);
    return !name || ANON_AGGRNAME_P (name);
}

} // namespace

namespace gcckdm
{

namespace kdmtriplewriter
{

KdmTripleWriter::KdmTripleWriter(KdmSinkPtr const & kdmSinkPtr) :
    mKdmSink(kdmSinkPtr), mKdmElementId(KdmElementId_DefaultStart)
{

}

KdmTripleWriter::KdmTripleWriter(boost::filesystem::path const & filename) :
    mKdmSink(new boost::filesystem::ofstream(filename)), mKdmElementId(KdmElementId_DefaultStart)
{
}

KdmTripleWriter::~KdmTripleWriter()
{
    mKdmSink->flush();
}

void KdmTripleWriter::startTranslationUnit(boost::filesystem::path const & file)
{
    mCompilationFile = file;
    writeVersionHeader();
    writeDefaultKdmModelElements();
    writeKdmSourceFile(mCompilationFile);
}

void KdmTripleWriter::startKdmGimplePass()
{
    //C Support..variables are stored in varpool... C++ we can use global_namespace
    struct varpool_node *pNode;
    FOR_EACH_STATIC_VARIABLE(pNode)
    {
        long unitId = writeKdmStorableUnit(pNode->decl);
        writeTripleContains(getSourceFileReferenceId(pNode->decl), unitId);
    }
}

void KdmTripleWriter::finishKdmGimplePass()
{
    for (; !mNodeQueue.empty(); mNodeQueue.pop())
    {
        processAstNode(mNodeQueue.front());
    }
}

void KdmTripleWriter::finishTranslationUnit()
{
    for (; !mNodeQueue.empty(); mNodeQueue.pop())
    {
        processAstNode(mNodeQueue.front());
    }

    for (TreeMap::const_iterator i = mReferencedSharedUnits.begin(), e = mReferencedSharedUnits.end(); i != e; ++i)
    {
        writeKdmSharedUnit(i->first);
    }
}

void KdmTripleWriter::processAstNode(tree const ast)
{
    //Ensure we haven't processed this node node before
    if (mProcessedNodes.find(ast) == mProcessedNodes.end())
    {
        int treeCode(TREE_CODE(ast));

        if (DECL_P(ast) && !DECL_IS_BUILTIN(ast))
        {
            processAstDeclarationNode(ast);
        }
        else if (treeCode == TREE_LIST)
        {
            //Not implemented yet but put here to prevent breakage
        }
        else if (TYPE_P(ast))
        {
            processAstTypeNode(ast);
        }
        else
        {
            std::cerr << "KdmTripleWriter: unsupported AST Node " << tree_code_name[treeCode] << std::endl;
        }
        mProcessedNodes.insert(ast);
    }
}

void KdmTripleWriter::processAstDeclarationNode(tree const decl)
{
    assert(DECL_P(decl));

    int treeCode(TREE_CODE(decl));
    switch (treeCode)
    {
        case VAR_DECL:
        {
            processAstVariableDeclarationNode(decl);
            break;
        }
        case FUNCTION_DECL:
        {
            processAstFunctionDeclarationNode(decl);
            break;
        }
        case FIELD_DECL:
        {
            processAstFieldDeclarationNode(decl);
            break;
        }
        case PARM_DECL:
        {
            //Not implemented yet but put here to prevent breakage
            break;
        }
        default:
        {
            std::cerr << "KdmTripleWriter: unsupported declaration node: " << tree_code_name[treeCode] << std::endl;
        }
    }
}

void KdmTripleWriter::processAstTypeNode(tree const typeNode)
{
    assert(TYPE_P(typeNode));

    if (typeNode == TYPE_MAIN_VARIANT(typeNode))
    {
        int treeCode(TREE_CODE(typeNode));
        switch (treeCode)
        {
            case ARRAY_TYPE:
            {
                writeKdmArrayType(typeNode);
                break;
            }
            case FUNCTION_TYPE:
            {
                writeKdmSignature(typeNode);
                break;
            }
            case POINTER_TYPE:
            {
                writeKdmPointerType(typeNode);
                break;
            }
            case VOID_TYPE:
                //Fall through
            case REAL_TYPE:
                //Fall through
            case INTEGER_TYPE:
            {
                writeKdmPrimitiveType(typeNode);
                break;
            }
            case UNION_TYPE:
                //Fall Through
            case RECORD_TYPE:
            {
                writeKdmRecordType(typeNode);
                break;
            }
            default:
            {
                std::cerr << "unsupported AST Type " << tree_code_name[treeCode] << std::endl;
                break;
            }
        }
    }
}

void KdmTripleWriter::processAstVariableDeclarationNode(tree const varDeclaration)
{
    writeKdmStorableUnit(varDeclaration);
}

void KdmTripleWriter::processAstFunctionDeclarationNode(tree const functionDecl)
{
    writeKdmCallableUnit(functionDecl);
}

void KdmTripleWriter::processAstFieldDeclarationNode(tree const fieldDecl)
{
    writeKdmItemUnit(fieldDecl);
}

void KdmTripleWriter::writeTriple(long const subject, KdmPredicate const & predicate, long const object)
{
    *mKdmSink << "<" << subject << "> <" << predicate << "> <" << object << ">.\n";
}

void KdmTripleWriter::writeTriple(long const subject, KdmPredicate const & predicate, KdmType const & object)
{
    *mKdmSink << "<" << subject << "> <" << predicate << "> \"" << object.name() << "\".\n";
}

void KdmTripleWriter::writeTriple(long const subject, KdmPredicate const & predicate, std::string const & object)
{
    *mKdmSink << "<" << subject << "> <" << predicate << "> \"" << object << "\".\n";
}

void KdmTripleWriter::writeKdmCallableUnit(tree const functionDecl)
{
    std::string name(nodeName(functionDecl));

    long callableUnitId = getReferenceId(functionDecl);
    writeTripleKdmType(callableUnitId, KdmType::CallableUnit());
    writeTripleName(callableUnitId, name);
    writeTripleLinkId(callableUnitId, name);

    std::string sourceFile(DECL_SOURCE_FILE(functionDecl));
    long unitId = (sourceFile == mCompilationFile.string()) ? KdmElementId_CompilationUnit : KdmElementId_ClassSharedUnit;
    writeTripleContains(unitId, callableUnitId);

    writeKdmSourceRef(callableUnitId, functionDecl);

    long signatureId = writeKdmSignature(functionDecl);
    writeTripleContains(callableUnitId, signatureId);

    if (gimple_has_body_p(functionDecl))
    {
        gimple_seq seq = gimple_body(functionDecl);
        processGimpleSequence(functionDecl, seq);
    }
}

long KdmTripleWriter::writeKdmSignatureDeclaration(tree const functionDecl)
{
    std::string name(nodeName(functionDecl));
    long signatureId = ++mKdmElementId;
    writeTripleKdmType(signatureId, KdmType::Signature());
    writeTripleName(signatureId, name);

    //Determine return type id
    tree t(TREE_TYPE (TREE_TYPE (functionDecl)));
    tree t2(TYPE_MAIN_VARIANT(t));
    long paramId = writeKdmReturnParameterUnit(t2);
    writeTripleContains(signatureId, paramId);

    //Iterator through argument list
    tree arg(DECL_ARGUMENTS (functionDecl));
    tree argType(TYPE_ARG_TYPES (TREE_TYPE (functionDecl)));
    while (argType && (argType != void_list_node))
    {
        long refId = writeKdmParameterUnit(arg);
        writeTripleContains(signatureId, refId);
        if (arg)
        {
            arg = TREE_CHAIN (arg);
        }
        argType = TREE_CHAIN (argType);
    }
    return signatureId;
}

long KdmTripleWriter::writeKdmSignatureType(tree const functionType)
{
    std::string name(nodeName(functionType));
    long signatureId = getReferenceId(functionType);
    writeTripleKdmType(signatureId, KdmType::Signature());
    writeTripleName(signatureId, name);

    //Determine return type
    long paramId = writeKdmReturnParameterUnit(TREE_TYPE(functionType));
    writeTripleContains(signatureId, paramId);

    //Iterator through argument list
    tree argType(TYPE_ARG_TYPES (functionType));
    while (argType && (argType != void_list_node))
    {
        long refId = writeKdmParameterUnit(argType);
        writeTripleContains(signatureId, refId);
        argType = TREE_CHAIN (argType);
    }
    writeTripleContains(KdmElementId_CompilationUnit, signatureId);

    return signatureId;

}

long KdmTripleWriter::writeKdmSignature(tree const function)
{
    long sigId;
    if (DECL_P(function))
    {
        sigId = writeKdmSignatureDeclaration(function);
    }
    else
    {
        sigId = writeKdmSignatureType(function);

    }
    return sigId;

}

void KdmTripleWriter::writeVersionHeader()
{
    *mKdmSink << "KDM_Triple:" << KdmTripleWriter::KdmTripleVersion << "\n";
}

void KdmTripleWriter::writeDefaultKdmModelElements()
{
    writeTriple(KdmElementId_Segment, KdmPredicate::KdmType(), KdmType::Segment());
    writeTriple(KdmElementId_Segment, KdmPredicate::Uid(), "0");
    writeTriple(KdmElementId_Segment, KdmPredicate::LinkId(), "root");
    writeTriple(KdmElementId_CodeModel, KdmPredicate::KdmType(), KdmType::CodeModel());
    writeTriple(KdmElementId_CodeModel, KdmPredicate::Name(), KdmType::CodeModel());
    writeTriple(KdmElementId_CodeModel, KdmPredicate::Uid(), "1");
    writeTriple(KdmElementId_CodeModel, KdmPredicate::LinkId(), KdmType::CodeModel());
    writeTriple(KdmElementId_Segment, KdmPredicate::Contains(), KdmElementId_CodeModel);
    writeTriple(KdmElementId_WorkbenchExtensionFamily, KdmPredicate::KdmType(), KdmType::ExtensionFamily());
    writeTriple(KdmElementId_WorkbenchExtensionFamily, KdmPredicate::Name(), "__WORKBENCH__");
    writeTriple(KdmElementId_WorkbenchExtensionFamily, KdmPredicate::LinkId(), "__WORKBENCH__");
    writeTriple(KdmElementId_Segment, KdmPredicate::Contains(), KdmElementId_WorkbenchExtensionFamily);
    writeTriple(KdmElementId_HiddenStereoType, KdmPredicate::KdmType(), KdmType::StereoType());
    writeTriple(KdmElementId_HiddenStereoType, KdmPredicate::Name(), "__HIDDEN__");
    writeTriple(KdmElementId_HiddenStereoType, KdmPredicate::LinkId(), "__HIDDEN__");
    writeTriple(KdmElementId_WorkbenchExtensionFamily, KdmPredicate::Contains(), KdmElementId_HiddenStereoType);
    writeTriple(KdmElementId_CodeAssembly, KdmPredicate::KdmType(), KdmType::CodeAssembly());
    writeTriple(KdmElementId_CodeAssembly, KdmPredicate::Name(), ":code");
    writeTriple(KdmElementId_CodeAssembly, KdmPredicate::Uid(), "2");
    writeTriple(KdmElementId_CodeAssembly, KdmPredicate::LinkId(), ":code");
    writeTriple(KdmElementId_CodeModel, KdmPredicate::Contains(), KdmElementId_CodeAssembly);
    writeTriple(KdmElementId_PrimitiveSharedUnit, KdmPredicate::KdmType(), KdmType::SharedUnit());
    writeTriple(KdmElementId_PrimitiveSharedUnit, KdmPredicate::Name(), ":primitive");
    writeTriple(KdmElementId_PrimitiveSharedUnit, KdmPredicate::Uid(), "3");
    writeTriple(KdmElementId_PrimitiveSharedUnit, KdmPredicate::LinkId(), ":primitive");
    writeTriple(KdmElementId_CodeAssembly, KdmPredicate::Contains(), KdmElementId_PrimitiveSharedUnit);
    writeTriple(KdmElementId_DerivedSharedUnit, KdmPredicate::KdmType(), KdmType::SharedUnit());
    writeTriple(KdmElementId_DerivedSharedUnit, KdmPredicate::Name(), ":derived");
    writeTriple(KdmElementId_DerivedSharedUnit, KdmPredicate::Uid(), "4");
    writeTriple(KdmElementId_DerivedSharedUnit, KdmPredicate::LinkId(), ":derived");
    writeTriple(KdmElementId_CodeAssembly, KdmPredicate::Contains(), KdmElementId_DerivedSharedUnit);
    writeTriple(KdmElementId_ClassSharedUnit, KdmPredicate::KdmType(), KdmType::SharedUnit());
    writeTriple(KdmElementId_ClassSharedUnit, KdmPredicate::Name(), ":class");
    writeTriple(KdmElementId_ClassSharedUnit, KdmPredicate::Uid(), "5");
    writeTriple(KdmElementId_ClassSharedUnit, KdmPredicate::LinkId(), ":class");
    writeTriple(KdmElementId_CodeAssembly, KdmPredicate::Contains(), KdmElementId_ClassSharedUnit);
    writeTriple(KdmElementId_InventoryModel, KdmPredicate::KdmType(), KdmType::InventoryModel());
    writeTriple(KdmElementId_InventoryModel, KdmPredicate::Name(), KdmType::InventoryModel());
    writeTriple(KdmElementId_InventoryModel, KdmPredicate::LinkId(), KdmType::InventoryModel());
    writeTriple(KdmElementId_Segment, KdmPredicate::Contains(), KdmElementId_InventoryModel);

    writeTripleKdmType(KdmElementId_CompilationUnit, KdmType::CompilationUnit());
    writeTripleName(KdmElementId_CompilationUnit, mCompilationFile.filename());
    writeTriple(KdmElementId_CompilationUnit, KdmPredicate::LinkId(), mCompilationFile.string());
    writeTriple(KdmElementId_CodeAssembly, KdmPredicate::Contains(), KdmElementId_CompilationUnit);

}

void KdmTripleWriter::writeTripleKdmType(long const subject, KdmType const & object)
{
    writeTriple(subject, KdmPredicate::KdmType(), object);
}

void KdmTripleWriter::writeTripleName(long const subject, std::string const & name)
{
    writeTriple(subject, KdmPredicate::Name(), name);
}

void KdmTripleWriter::writeTripleContains(long const parent, long const child)
{
    writeTriple(parent, KdmPredicate::Contains(), child);
}
void KdmTripleWriter::writeTripleLinkId(long const subject, std::string const & name)
{
    writeTriple(subject, KdmPredicate::LinkId(), name);
}

void KdmTripleWriter::writeKdmSourceFile(boost::filesystem::path const & file)
{
    writeTripleKdmType(++mKdmElementId, KdmType::SourceFile());
    writeTripleName(mKdmElementId, file.filename());
    writeTriple(mKdmElementId, KdmPredicate::Path(), file.string());
    writeTripleLinkId(mKdmElementId, file.string());
    writeTripleContains(KdmElementId_InventoryModel, mKdmElementId);

    //Keep track of all files inserted into the inventory model
    mInventoryMap.insert(std::make_pair(file, mKdmElementId));
}

void KdmTripleWriter::writeKdmCompilationUnit(boost::filesystem::path const & file)
{
    writeTripleKdmType(++mKdmElementId, KdmType::CompilationUnit());
    writeTripleName(mKdmElementId, file.filename());
    writeTripleLinkId(mKdmElementId, file.string());
    writeTripleContains(KdmElementId_CodeAssembly, mKdmElementId);
}

long KdmTripleWriter::writeKdmReturnParameterUnit(tree const param)
{
    long ref = getReferenceId(param);
    writeTripleKdmType(++mKdmElementId, KdmType::ParameterUnit());
    writeTripleName(mKdmElementId, "__RESULT__");
    writeTriple(mKdmElementId, KdmPredicate::Type(), ref);
    return mKdmElementId;
}

long KdmTripleWriter::writeKdmParameterUnit(tree const param)
{
    long parameterUnitId = getReferenceId(param);
    writeTripleKdmType(parameterUnitId, KdmType::ParameterUnit());

    tree type = TREE_TYPE(param) ? TYPE_MAIN_VARIANT(TREE_TYPE(param)) : TREE_VALUE (param);
    long ref = getReferenceId(type);

    std::string name(nodeName(param));
    writeTripleName(parameterUnitId, name);

    writeTriple(parameterUnitId, KdmPredicate::Type(), ref);
    return parameterUnitId;
}

long KdmTripleWriter::writeKdmItemUnit(tree const item)
{
    long itemId = getReferenceId(item);
    writeTripleKdmType(itemId, KdmType::ItemUnit());
    tree type(TYPE_MAIN_VARIANT(TREE_TYPE(item)));
    long ref = getReferenceId(type);
    std::string name(nodeName(item));

    writeTripleName(itemId, name);
    writeTriple(itemId, KdmPredicate::Type(), ref);
    writeKdmSourceRef(itemId, item);
    return itemId;
}

long KdmTripleWriter::writeKdmStorableUnit(tree const var)
{
    long unitId = getReferenceId(var);
    writeTripleKdmType(unitId, KdmType::StorableUnit());
    writeTripleName(unitId, nodeName(var));
    tree type(TYPE_MAIN_VARIANT(TREE_TYPE(var)));
    long ref = getReferenceId(type);
    writeTriple(unitId, KdmPredicate::Type(), ref);
    writeKdmSourceRef(unitId, var);

    return unitId;
}

std::string KdmTripleWriter::getUnaryRhsString(gimple const gs)
{
    std::string rhsString("");

    enum tree_code rhs_code = gimple_assign_rhs_code(gs);
    tree lhs = gimple_assign_lhs(gs);
    tree rhs = gimple_assign_rhs1(gs);

    switch (rhs_code)
    {
        case VIEW_CONVERT_EXPR:
        case ASSERT_EXPR:
            rhsString += nodeName(rhs);
            break;

        case FIXED_CONVERT_EXPR:
        case ADDR_SPACE_CONVERT_EXPR:
        case FIX_TRUNC_EXPR:
        case FLOAT_EXPR:
        CASE_CONVERT
        :
            rhsString += "(" + nodeName(TREE_TYPE(lhs)) + ") ";
            if (op_prio(rhs) < op_code_prio(rhs_code))
            {
                rhsString += "(" + nodeName(rhs) + ")";
            }
            else
                rhsString += nodeName(rhs);
            break;

        case PAREN_EXPR:
            rhsString += "((" + nodeName(rhs) + "))";
            break;

        case ABS_EXPR:
            rhsString += "ABS_EXPR <" + nodeName(rhs) + ">";
            break;

        default:
            if (TREE_CODE_CLASS (rhs_code) == tcc_declaration || TREE_CODE_CLASS (rhs_code) == tcc_constant || TREE_CODE_CLASS (rhs_code)
                    == tcc_reference || rhs_code == SSA_NAME || rhs_code == ADDR_EXPR || rhs_code == CONSTRUCTOR)
            {
                rhsString += nodeName(rhs);
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
                rhsString += "(" + nodeName(rhs) + ")";
            }
            else
            {
                rhsString += nodeName(rhs);
            }
            break;
    }
    return rhsString;
}

std::string KdmTripleWriter::getBinaryRhsString(gimple const gs)
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
            rhsString += tree_code_name[static_cast<int>(code)];
            std::transform(rhsString.begin(), rhsString.end(), rhsString.begin(), toupper);
            //        for (p = tree_code_name [(int) code]; *p; p++)
            //      pp_character (buffer, TOUPPER (*p));
            rhsString += " <" + nodeName(gimple_assign_rhs1(gs)) + ", " + nodeName(gimple_assign_rhs2(gs)) + ">";
            break;
        }
        default:
        {
            if (op_prio(gimple_assign_rhs1(gs)) <= op_code_prio(code))
            {
                rhsString += "(" + nodeName(gimple_assign_rhs1(gs)) + ")";
            }
            else
            {
                rhsString += nodeName(gimple_assign_rhs1(gs)) + " " + std::string(op_symbol_code(gimple_assign_rhs_code(gs))) + " ";
            }
            if (op_prio(gimple_assign_rhs2(gs)) <= op_code_prio(code))
            {
                rhsString += "(" + nodeName(gimple_assign_rhs2(gs)) + ")";
            }
            else
            {
                rhsString += nodeName(gimple_assign_rhs2(gs));
            }
        }
    }
    return rhsString;

}

std::string KdmTripleWriter::getTernaryRhsString(gimple const gs)
{
    std::cerr << "TernaryRhsString not implemented" << std::endl;
    return "<TODO: ternary not implemented>";
//    ///Might not need this function I don't know
//
//    std::string rhsString("");
////    const char *p;
//    enum tree_code code = gimple_assign_rhs_code (gs);
//    switch (code)
//      {
//      case WIDEN_MULT_PLUS_EXPR:
//      case WIDEN_MULT_MINUS_EXPR:
//      {
//          rhsString += tree_code_name [static_cast<int>(code)];
//          std::transform(rhsString.begin(), rhsString.end(), rhsString.begin(), toupper);
//          rhsString += " <" + nodeName(gimple_assign_rhs1(gs)) + ", " + nodeName(gimple_assign_rhs2(gs)) + ", " + nodeName(gimple_assign_rhs3(gs)) + ">";
//        break;
//      }
//
//      default:
//      {
//        gcc_unreachable ();
//      }

}

long KdmTripleWriter::writeKdmActionElement(gimple const gs)
{
    long id = ++mKdmElementId;
    writeTripleKdmType(id, KdmType::ActionElement());
    writeTriple(id, KdmPredicate::Kind(), "Assign");

    std::string nameStr("");
    nameStr += nodeName(gimple_assign_lhs(gs)) + " = ";

    unsigned numOps = gimple_num_ops(gs);
    if (numOps == 2)
    {
        nameStr += getUnaryRhsString(gs);
    }
    else if (numOps == 3)
    {
        nameStr += getBinaryRhsString(gs);
    }
    else if (numOps == 4)
    {
        nameStr += getTernaryRhsString(gs);
    }

    writeTripleName(id, nameStr);
    writeTripleLinkId(id, nodeName(gimple_assign_lhs(gs)));
    return id;
}

long KdmTripleWriter::getReferenceId(tree const node)
{
    long retValue(-1);
    std::pair<TreeMap::iterator, bool> result = mReferencedNodes.insert(std::make_pair(node, mKdmElementId + 1));
    if (result.second)
    {
        //if we haven't processed this node before new node....queue it node for later processing
        if (mProcessedNodes.find(node) == mProcessedNodes.end())
        {
            mNodeQueue.push(node);
        }

        retValue = ++mKdmElementId;
        tree treeType(TREE_TYPE(node));
        if (treeType)
        {
            tree t2(TYPE_MAIN_VARIANT(treeType));
            //reserve the id for this type for later processing
            getReferenceId(t2);
        }
    }
    else
    {
        retValue = result.first->second;
    }
    return retValue;
}

long KdmTripleWriter::getSourceFileReferenceId(tree const t)
{
    long sourceFileId(0);
    expanded_location loc(expand_location(locationOf(t)));
    if (mCompilationFile != boost::filesystem::path(loc.file))
    {
        tree t = get_identifier(loc.file);
        sourceFileId = getSharedUnitReferenceId(t);
    }
    else
    {
        sourceFileId = KdmElementId_CompilationUnit;
    }
    return sourceFileId;
}

long KdmTripleWriter::getSharedUnitReferenceId(tree const file)
{
    long retValue(-1);
    std::pair<TreeMap::iterator, bool> result = mReferencedSharedUnits.insert(std::make_pair(file, mKdmElementId + 1));
    if (result.second)
    {
        retValue = ++mKdmElementId;
    }
    else
    {
        retValue = result.first->second;
    }
    return retValue;
}

void KdmTripleWriter::writeKdmPrimitiveType(tree const type)
{
    long typeKdmElementId = getReferenceId(type);

    std::string name(nodeName(type));

    KdmType kdmType = KdmType::PrimitiveType();
    if (name.find("int") != std::string::npos || name.find("long") != std::string::npos)
    {
        kdmType = KdmType::IntegerType();
    }
    else if (name.find("double") != std::string::npos)
    {
        kdmType = KdmType::DecimalType();
    }
    else if (name.find("void") != std::string::npos)
    {
        kdmType = KdmType::VoidType();
    }
    else if (name.find("float") != std::string::npos)
    {
        kdmType = KdmType::FloatType();
    }
    else if (name.find("char") != std::string::npos)
    {
        kdmType = KdmType::CharType();
    }

    writeTripleKdmType(typeKdmElementId, kdmType);
    writeTripleName(typeKdmElementId, name);
}

void KdmTripleWriter::writeKdmPointerType(tree const pointerType)
{
    long pointerKdmElementId = getReferenceId(pointerType);
    writeTripleKdmType(pointerKdmElementId, KdmType::PointerType());
    writeTripleName(pointerKdmElementId, "PointerType");

    tree treeType(TREE_TYPE(pointerType));
    tree t2(TYPE_MAIN_VARIANT(treeType));
    long pointerTypeKdmElementId = getReferenceId(t2);
    processAstNode(t2);

    writeTriple(pointerKdmElementId, KdmPredicate::Type(), pointerTypeKdmElementId);

}

void KdmTripleWriter::writeKdmArrayType(tree const arrayType)
{
    long arrayKdmElementId = getReferenceId(arrayType);
    writeTripleKdmType(arrayKdmElementId, KdmType::ArrayType());

    tree treeType(TREE_TYPE(arrayType));
    tree t2(TYPE_MAIN_VARIANT(treeType));
    long arrayTypeKdmElementId = getReferenceId(t2);
    writeTriple(arrayKdmElementId, KdmPredicate::Type(), arrayTypeKdmElementId);

}

void KdmTripleWriter::writeKdmRecordType(tree const recordType)
{
    tree mainRecordType = TYPE_MAIN_VARIANT (recordType);

    if (TREE_CODE(mainRecordType) == ENUMERAL_TYPE)
    {
        std::cerr << "Unhandled Enumeral type" << std::endl;
        //enum
    }
    else if (global_namespace && TYPE_LANG_SPECIFIC (mainRecordType) && CLASSTYPE_DECLARED_CLASS (mainRecordType))
    {
        std::cerr << "Unhandled Class type" << std::endl;
        //class
    }
    else //Record or Union

    {
        long compilationUnitId(getSourceFileReferenceId(mainRecordType));

        //struct
        long structId = getReferenceId(mainRecordType);
        writeTripleKdmType(structId, KdmType::RecordType());
        std::string name;
        //check to see if we are an annonymous struct
        name = (isAnonymousStruct(mainRecordType)) ? unamedNode : nodeName(mainRecordType);
        writeTripleName(structId, name);

        if (COMPLETE_TYPE_P (mainRecordType))
        {
            for (tree d(TYPE_FIELDS(mainRecordType)); d; d = TREE_CHAIN(d))
            {
                switch (TREE_CODE(d))
                {
                    case TYPE_DECL:
                    {
                        if (!DECL_SELF_REFERENCE_P(d))
                        {
                            std::cerr << "Unimplemented feature" << std::endl;
                        }
                        break;
                    }
                    case FIELD_DECL:
                    {
                        if (!DECL_ARTIFICIAL(d))
                        {
                            long itemId = getReferenceId(d);
                            processAstNode(d);
                            writeTripleContains(structId, itemId);
                        }
                        break;
                    }
                    default:
                    {
                        std::cerr << "Unimplemented feature" << std::endl;
                        break;
                    }
                }
            }

        }

        writeKdmSourceRef(structId, mainRecordType);

        writeTripleContains(compilationUnitId, structId);
    }
    //    std::cerr << "======================= End of Record Type\n";

}

void KdmTripleWriter::writeKdmSharedUnit(tree const file)
{
    long id = getSharedUnitReferenceId(file);
    writeTripleKdmType(id, KdmType::SharedUnit());

    boost::filesystem::path filename(IDENTIFIER_POINTER(file));
    writeTripleName(id, filename.filename());
    writeTripleLinkId(id, filename.string());
    writeTripleContains(KdmElementId_CodeAssembly, id);
}

long KdmTripleWriter::writeKdmSourceRef(long id, const tree node)
{
	return writeKdmSourceRef(id, locationOf(node));
}

long KdmTripleWriter::writeKdmSourceRef(long id, const location_t loc)
{
    expanded_location eloc = expand_location(loc);
    Path filename(eloc.file);
    FileMap::iterator i = mInventoryMap.find(filename);
    if (i == mInventoryMap.end())
    {
        writeKdmSourceFile(filename);
        i = mInventoryMap.find(filename);
    }

    std::string srcRef = boost::lexical_cast<std::string>(i->second) + ";" + boost::lexical_cast<std::string>(eloc.line);
    writeTriple(id, KdmPredicate::SourceRef(), srcRef);

    return id;
}

void gimple_not_implemented_yet(gimple const gs)
{
    std::cerr << "Unknown GIMPLE statement: " << gimple_code_name[static_cast<int> (gimple_code(gs))] << std::endl;
    print_gimple_stmt(stderr, gs, 0, 0);
}

void KdmTripleWriter::processGimpleSequence(tree const parent, gimple_seq const seq)
{
    for (gimple_stmt_iterator i = gsi_start(seq); !gsi_end_p(i); gsi_next(&i))
    {
        gimple gs = gsi_stmt(i);
        processGimpleStatement(parent, gs);
    }
}

void KdmTripleWriter::processGimpleStatement(tree const parent, gimple const gs)
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

void KdmTripleWriter::processGimpleBindStatement(tree const parent, gimple const gs)
{
    tree var;
    for (var = gimple_bind_vars(gs); var; var = TREE_CHAIN (var))
    {
        long declId = getReferenceId(var);
        processAstNode(var);
        writeTripleContains(getReferenceId(parent), declId);
    }

    processGimpleSequence(parent, gimple_bind_body(gs));
}

long KdmTripleWriter::getBlockReferenceId(location_t const loc)
{
    LocationMap::iterator i = mBlockUnitMap.find(loc);
    long blockId;
    if (i == mBlockUnitMap.end())
    {
    	blockId = ++mKdmElementId;
        mBlockUnitMap.insert(std::make_pair(loc, blockId));
        writeTripleKdmType(blockId, KdmType::BlockUnit());
        long srcId = writeKdmSourceRef(blockId, loc);
        writeTripleContains(blockId, srcId);
    }
    else
    {
    	blockId = i->second;
    }
    return blockId;
}

void KdmTripleWriter::processGimpleAssignStatement(tree const parent, gimple const gs)
{
	long blockId = getBlockReferenceId(gimple_location(gs));
    long actionId = writeKdmActionElement(gs);
    writeTripleContains(blockId, actionId);
}

} // namespace kdmtriplewriter

} // namespace gcckdm
