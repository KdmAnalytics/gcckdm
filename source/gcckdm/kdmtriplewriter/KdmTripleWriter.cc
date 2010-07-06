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
std::string nodeName(tree node)
{
//    std::cerr << "GCC:";
//    debug_generic_stmt(node);
//    std::cerr << "Mine:" << gcckdm::getAstNodeName(node) << std::endl;
//    std::string name(gcckdm::treeNodeNameString(node));
//    long uid = static_cast<long>(DECL_P(node) ? DECL_UID(node) : TYPE_UID(node));
//    return name.empty() ? boost::lexical_cast<std::string>(uid) : name;
    return gcckdm::getAstNodeName(node);
}

/**
 * True if the given AST node is an annoymous struct
 *
 * @param t the node to test
 * @return true if t is an annoymous struct
 */
bool isAnonymousStruct(tree t)
{
    tree name = TYPE_NAME (t);
    if (name && TREE_CODE (name) == TYPE_DECL)
        name = DECL_NAME (name);
    return !name || ANON_AGGRNAME_P (name);
}

}

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
    for (AstNodeReferenceMap::const_iterator i = mReferencedNodes.begin(), e = mReferencedNodes.end(); i != e; ++i)
    {
        processAstNode(i->first);
    }
}

void KdmTripleWriter::finishTranslationUnit()
{
    //    for (TreeSet::iterator i = mProcessedNodes.begin(); i!= mProcessedNodes.end(); ++i)
    //    {
    //        std::cerr << nodeName(*i) << std::endl;
    //    }
    //    std::cerr << "=======================" << std::endl;
    do
    {
        for (AstNodeReferenceMap::const_iterator i = mReferencedNodes.begin(), e = mReferencedNodes.end(); i != e; ++i)
        {
            //            std::cerr << i->second << std::endl;
            if (mProcessedNodes.find(i->first) == mProcessedNodes.end())
            {
                processAstNode(i->first);
            }
        }
        //        std::cerr << "============================\n" << std::endl;
    }
    while (mProcessedNodes.size() != mReferencedNodes.size());

    for (AstNodeReferenceMap::const_iterator i = mReferencedSharedUnits.begin(), e = mReferencedSharedUnits.end(); i != e; ++i)
    {
        writeKdmSharedUnit(i->first);
    }

}

void KdmTripleWriter::processAstNode(tree ast)
{
    int treeCode(TREE_CODE(ast));

    if (DECL_P(ast) && !DECL_IS_BUILTIN(ast))
    {
        processAstDeclarationNode(ast);
        mProcessedNodes.insert(ast);
    }
    else if (treeCode == TREE_LIST)
    {
        //Not implemented yet but put here to prevent breakage
        mProcessedNodes.insert(ast);
    }
    else if (TYPE_P(ast))
    {
        processAstTypeNode(ast);
        mProcessedNodes.insert(ast);
    }
    else
    {
        std::cerr << "KdmTripleWriter: unsupported AST Node " << tree_code_name[treeCode] << std::endl;
        mProcessedNodes.insert(ast);
    }
}

void KdmTripleWriter::processAstDeclarationNode(tree decl)
{
    assert(DECL_P(decl));

    //Ensure we haven't processed this declaration node before
    if (mProcessedNodes.find(decl) == mProcessedNodes.end())
    {
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
}

void KdmTripleWriter::processAstTypeNode(tree typeNode)
{
    assert(TYPE_P(typeNode));

    //Ensure that we haven't processed this type before
    if (mProcessedNodes.find(typeNode) == mProcessedNodes.end())
    {
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
}

void KdmTripleWriter::processAstVariableDeclarationNode(tree varDeclaration)
{
    writeKdmStorableUnit(varDeclaration);
}

void KdmTripleWriter::processAstFunctionDeclarationNode(tree functionDecl)
{
    writeKdmCallableUnit(functionDecl);
}

void KdmTripleWriter::processAstFieldDeclarationNode(tree fieldDecl)
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

void KdmTripleWriter::writeKdmCallableUnit(tree functionDecl)
{
    std::string name(nodeName(functionDecl));

    long callableUnitId = getReferenceId(functionDecl);
    writeTripleKdmType(callableUnitId, KdmType::CallableUnit());
    writeTripleName(callableUnitId, name);
    writeTripleLinkId(callableUnitId, name);

    std::string sourceFile(DECL_SOURCE_FILE(functionDecl));
    long unitId = (sourceFile == mCompilationFile.string()) ? KdmElementId_CompilationUnit : KdmElementId_ClassSharedUnit;
    writeTripleContains(unitId, callableUnitId);

    long signatureId = writeKdmSignature(functionDecl);
    writeTripleContains(callableUnitId, signatureId);

    if (gimple_has_body_p(functionDecl))
    {
        gimple_seq seq = gimple_body(functionDecl);

        for (gimple_stmt_iterator i = gsi_start(seq); !gsi_end_p(i); gsi_next(&i))
        {
            gimple gs = gsi_stmt(i);
            processGimpleStatement(functionDecl, gs);
        }
    }
}

long KdmTripleWriter::writeKdmSignatureDeclaration(tree functionDecl)
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

long KdmTripleWriter::writeKdmSignatureType(tree functionType)
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

long KdmTripleWriter::writeKdmSignature(tree function)
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
}

void KdmTripleWriter::writeKdmCompilationUnit(boost::filesystem::path const & file)
{
    writeTripleKdmType(++mKdmElementId, KdmType::CompilationUnit());
    writeTripleName(mKdmElementId, file.filename());
    writeTripleLinkId(mKdmElementId, file.string());
    writeTripleContains(KdmElementId_CodeAssembly, mKdmElementId);
}

long KdmTripleWriter::writeKdmReturnParameterUnit(tree param)
{
    long ref = getReferenceId(param);
    writeTripleKdmType(++mKdmElementId, KdmType::ParameterUnit());
    writeTripleName(mKdmElementId, "__RESULT__");
    writeTriple(mKdmElementId, KdmPredicate::Type(), ref);
    return mKdmElementId;
}

long KdmTripleWriter::writeKdmParameterUnit(tree param)
{
    long parameterUnitId = getReferenceId(param);
    writeTripleKdmType(parameterUnitId, KdmType::ParameterUnit());

    tree type = TREE_TYPE(param) ? TYPE_MAIN_VARIANT(TREE_TYPE(param)): TREE_VALUE (param);
    long ref = getReferenceId(type);

    std::string name(nodeName(param));
    writeTripleName(parameterUnitId, name);

    writeTriple(parameterUnitId, KdmPredicate::Type(), ref);
    return parameterUnitId;
}

long KdmTripleWriter::writeKdmItemUnit(tree item)
{
    long itemId = getReferenceId(item);
    writeTripleKdmType(itemId, KdmType::ItemUnit());
    tree type(TYPE_MAIN_VARIANT(TREE_TYPE(item)));
    long ref = getReferenceId(type);
    std::string name(nodeName(item));

    writeTripleName(itemId, name);
    writeTriple(itemId, KdmPredicate::Type(), ref);
    return itemId;
}

long KdmTripleWriter::writeKdmStorableUnit(tree var)
{
    long unitId = getReferenceId(var);
    writeTripleKdmType(unitId, KdmType::StorableUnit());
    writeTripleName(unitId, nodeName(var));
    tree type(TYPE_MAIN_VARIANT(TREE_TYPE(var)));
    long ref = getReferenceId(type);
    writeTriple(unitId, KdmPredicate::Type(), ref);
    return unitId;
}

long KdmTripleWriter::getReferenceId(tree node)
{
    long retValue(-1);
    std::pair<AstNodeReferenceMap::iterator, bool> result = mReferencedNodes.insert(std::make_pair(node, mKdmElementId + 1));
    if (result.second)
    {
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

long KdmTripleWriter::getSourceFileReferenceId(tree t)
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

long KdmTripleWriter::getSharedUnitReferenceId(tree file)
{
    long retValue(-1);
    std::pair<AstNodeReferenceMap::iterator, bool> result = mReferencedSharedUnits.insert(std::make_pair(file, mKdmElementId + 1));
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

void KdmTripleWriter::writeKdmPrimitiveType(tree type)
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

void KdmTripleWriter::writeKdmPointerType(tree pointerType)
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

void KdmTripleWriter::writeKdmArrayType(tree arrayType)
{
    long arrayKdmElementId = getReferenceId(arrayType);
    writeTripleKdmType(arrayKdmElementId, KdmType::ArrayType());

    tree treeType(TREE_TYPE(arrayType));
    tree t2(TYPE_MAIN_VARIANT(treeType));
    long arrayTypeKdmElementId = getReferenceId(t2);
    writeTriple(arrayKdmElementId, KdmPredicate::Type(), arrayTypeKdmElementId);

}

void KdmTripleWriter::writeKdmRecordType(tree recordType)
{
    recordType = TYPE_MAIN_VARIANT (recordType);

    if (TREE_CODE(recordType) == ENUMERAL_TYPE)
    {
        std::cerr << "Unhandled Enumeral type" << std::endl;
        //enum
    }
    else if (global_namespace && TYPE_LANG_SPECIFIC (recordType) && CLASSTYPE_DECLARED_CLASS (recordType))
    {
        std::cerr << "Unhandled Class type" << std::endl;
        //class
    }
    else //Record or Union
    {
        long compilationUnitId(getSourceFileReferenceId(recordType));

        //struct
        long structId = getReferenceId(recordType);
        writeTripleKdmType(structId, KdmType::RecordType());
        std::string name;
        //check to see if we are an annonymous struct
        name = (isAnonymousStruct(recordType)) ? unamedNode : nodeName(recordType);
        writeTripleName(structId, name);

        if (COMPLETE_TYPE_P (recordType))
        {
            for (tree d(TYPE_FIELDS(recordType)); d; d = TREE_CHAIN(d))
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
                            long itemId = writeKdmItemUnit(d);
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

        writeTripleContains(compilationUnitId, structId);
    }
}

void KdmTripleWriter::writeKdmSharedUnit(tree file)
{
    long id = getSharedUnitReferenceId(file);
    writeTripleKdmType(id, KdmType::SharedUnit());

    boost::filesystem::path filename(IDENTIFIER_POINTER(file));
    writeTripleName(id, filename.filename());
    writeTripleLinkId(id, filename.string());
    writeTripleContains(KdmElementId_CodeAssembly, id);
}


void gimple_not_implemented_yet(gimple gs)
{
    std::cerr << "Unknown GIMPLE statement: " << gimple_code_name[static_cast<int>(gimple_code(gs))] << std::endl;
}

void KdmTripleWriter::processGimpleStatement(tree parent, gimple gs)
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
                gimple_not_implemented_yet(gs);
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

void KdmTripleWriter::processGimpleBindStatement(tree parent, gimple gs)
{
    tree var;
    for (var = gimple_bind_vars (gs); var; var = TREE_CHAIN (var))
    {
        long declId = getReferenceId(var);
        processAstDeclarationNode(var);
        writeTripleContains(getReferenceId(parent), declId);
    }
}


} // namespace kdmtriplewriter

} // namespace gcckdm
