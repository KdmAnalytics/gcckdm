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

#include "gcckdm/GccKdmConfig.hh"
#include "gcckdm/KdmPredicate.hh"


namespace
{

std::string nodeName(tree node)
{
    std::string name(gcckdm::treeNodeNameString(node));
    return name.empty() ? "<unnamed>" : name;
}

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
        processAstNode(pNode->decl);
    }
}

void KdmTripleWriter::finishKdmGimplePass()
{
    for (AstNodeReferenceMap::const_iterator i = mReferencedNodes.begin(), e = mReferencedNodes.end(); i != e; ++i)
    {
        processAstNode(i->first);
        mProcessedNodes.insert(*i);
    }


}

void KdmTripleWriter::processAstNode(tree ast)
{
    int treeCode(TREE_CODE(ast));

    if (DECL_P(ast) && !DECL_IS_BUILTIN(ast))
    {
        processAstDeclarationNode(ast);
    }
    else if (TYPE_P(ast))
    {
        processAstTypeNode(ast);
    }
    else
    {
        std::cerr << "unsupported AST Node " << tree_code_name[treeCode] << std::endl;
    }
}

void KdmTripleWriter::processAstDeclarationNode(tree decl)
{
    assert(DECL_P(decl));

    //Ensure we haven't processed this declaration node before
    if (mDeclarationNodes.insert(decl).second)
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
            default:
            {
                std::cerr << "unsupported declaration node: " << tree_code_name[treeCode] << std::endl;
            }
        }
    }
}

void KdmTripleWriter::processAstTypeNode(tree typeNode)
{
    assert(TYPE_P(typeNode));

    //Ensure that we haven't processed this type before
    if (mTypeNodes.insert(typeNode).second)
    {
        if (COMPLETE_TYPE_P(typeNode))
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
                    case POINTER_TYPE:
                    {
                        writeKdmPointerType(typeNode);
                        break;
                    }
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

void KdmTripleWriter::finishTranslationUnit()
{
    do
    {
        for (AstNodeReferenceMap::const_iterator i = mReferencedNodes.begin(), e = mReferencedNodes.end(); i != e; ++i)
        {
            if (mProcessedNodes.find(i->first) == mProcessedNodes.end())
            {
                processAstNode(i->first);
                mProcessedNodes.insert(*i);
            }
        }
    }
    while (mProcessedNodes.size() != mReferencedNodes.size());


    for (AstNodeReferenceMap::const_iterator i = mReferencedSharedUnits.begin(), e = mReferencedSharedUnits.end(); i != e; ++i)
    {
        writeKdmSharedUnit(i->first);
    }

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
    tree id(DECL_NAME (functionDecl));
    std::string name(id ? IDENTIFIER_POINTER (id) : "<unnamed>");

    long callableUnitId = ++mKdmElementId;
    writeTripleKdmType(callableUnitId, KdmType::CallableUnit());
    writeTripleName(callableUnitId, name);
    writeTripleLinkId(callableUnitId, name);

    std::string sourceFile(DECL_SOURCE_FILE(functionDecl));
    if (sourceFile == mCompilationFile.string())
    {
        writeTripleContains(KdmElementId_CompilationUnit, callableUnitId);
    }
    else
    {
        writeTripleContains(KdmElementId_ClassSharedUnit, callableUnitId);
    }

    long signatureId = ++mKdmElementId;
    writeTripleKdmType(signatureId, KdmType::Signature());
    writeTripleName(signatureId, name);
    writeTripleContains(callableUnitId, signatureId);

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
    long parameterUnitId(++mKdmElementId);
    writeTripleKdmType(parameterUnitId, KdmType::ParameterUnit());
    tree type(TYPE_MAIN_VARIANT(TREE_TYPE(param)));
    long ref = getReferenceId(type);

    tree id(DECL_NAME (param));
    std::string name(id ? IDENTIFIER_POINTER (id) : "<unnamed>");
    writeTripleName(parameterUnitId, name);

    writeTriple(parameterUnitId, KdmPredicate::Type(), ref);
    return parameterUnitId;
}

long KdmTripleWriter::writeKdmItemUnit(tree item)
{
    long itemId(++mKdmElementId);
    writeTripleKdmType(itemId, KdmType::ItemUnit());
    tree type(TYPE_MAIN_VARIANT(TREE_TYPE(item)));
    long ref = getReferenceId(type);
    tree id(DECL_NAME (item));
    std::string name(id ? IDENTIFIER_POINTER (id) : "<unnamed>");
    writeTripleName(itemId, name);
    writeTriple(itemId, KdmPredicate::Type(), ref);
    return itemId;
}

void KdmTripleWriter::writeKdmStorableUnit(tree var)
{
    long unitId(++mKdmElementId);
    writeTripleKdmType(unitId, KdmType::StorableUnit());

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
            getReferenceId(t2);
        }
    }
    else
    {
        retValue = result.first->second;
    }
    return retValue;
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

    tree typeName(TYPE_NAME (type));
    tree treeName(NULL_TREE);
    //Some types do not have names...
    if (typeName)
    {
        treeName = (TREE_CODE(typeName) == IDENTIFIER_NODE) ? typeName : DECL_NAME (typeName);
    }
    std::string name(treeName ? IDENTIFIER_POINTER (treeName) : "<unnamed>");

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
        long compilationUnitId(0);
        expanded_location loc(expand_location(locationOf(recordType)));
        if (mCompilationFile != boost::filesystem::path(loc.file))
        {
            tree t = get_identifier(loc.file);
            compilationUnitId = getSharedUnitReferenceId(t);
        }
        else
        {
            compilationUnitId = KdmElementId_CompilationUnit;
        }

        //struct
        long structId = getReferenceId(recordType);
        writeTripleKdmType(structId, KdmType::RecordType());
        std::string name;
        //check to see if we are an annonymous struct
        name = (isAnonymousStruct(recordType)) ? "<unnamed>" : typeNameString(recordType);
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

} // namespace kdmtriplewriter

} // namespace gcckdm
