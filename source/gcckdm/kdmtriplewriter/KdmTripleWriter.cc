/*
 * KdmTripleWriter.cc
 *
 *  Created on: Jun 21, 2010
 *      Author: kgirard
 */

#include "gcckdm/kdmtriplewriter/KdmTripleWriter.hh"

#include "boost/filesystem/fstream.hpp"
#include "boost/filesystem/operations.hpp"

#include "gcckdm/GccKdmConfig.hh"
#include "gcckdm/KdmPredicate.hh"

namespace gcckdm
{

namespace kdmtriplewriter
{
KdmTripleWriter::KdmTripleWriter(KdmSinkPtr const & kdmSinkPtr) :
    mKdmSink(kdmSinkPtr), mSubjectId(SubjectId_DefaultStart)
{

}

KdmTripleWriter::KdmTripleWriter(boost::filesystem::path const & filename) :
    mKdmSink(new boost::filesystem::ofstream(filename)), mSubjectId(SubjectId_DefaultStart)
{
}

KdmTripleWriter::~KdmTripleWriter()
{
    mKdmSink->flush();
}

void KdmTripleWriter::startTranslationUnit(boost::filesystem::path const & file)
{
    mCompilationFile = file;
    writeTripleKdmHeader();
    writeDefaultKdmModelElements();
    writeSourceFile(mCompilationFile);
    //   writeCompilationUnit(file);
}

void KdmTripleWriter::startKdmGimplePass()
{

}

void KdmTripleWriter::finishKdmGimplePass()
{
    for (AstNodeReferenceMap::const_iterator i = referencedNodes.begin(), e = referencedNodes.end(); i != e; ++i)
    {
        processAstNode(i->first);
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
            case FUNCTION_DECL:
            {
                processAstFunctionDeclarationNode(decl);
                break;
            }
            default:
            {
                std::cerr << "unsupported declaration node" << tree_code_name[treeCode] << std::endl;
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
                    case POINTER_TYPE:
                    {
                        writePointerType(typeNode);
                        break;
                    }
                    case INTEGER_TYPE:
                    {
                        writePrimitiveType(typeNode);
                        break;
                    }
                    case RECORD_TYPE:
                    {
                        writeRecordType(typeNode);
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

void KdmTripleWriter::processAstFunctionDeclarationNode(tree functionDecl)
{
    writeCallableUnit(functionDecl);
}

void KdmTripleWriter::finishTranslationUnit()
{
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

void KdmTripleWriter::writeCallableUnit(tree functionDecl)
{
    tree id(DECL_NAME (functionDecl));
    std::string name(id ? IDENTIFIER_POINTER (id) : "<unnamed>");

    long callableUnitId = ++mSubjectId;
    writeKdmType(callableUnitId, KdmType::CallableUnit());
    writeName(callableUnitId, name);
    writeLinkId(callableUnitId, name);

    std::string sourceFile(DECL_SOURCE_FILE(functionDecl));
    if (sourceFile == mCompilationFile.string())
    {
        writeContains(SubjectId_CompilationUnit, callableUnitId);
    }
    else
    {
        writeContains(SubjectId_ClassSharedUnit, callableUnitId);
    }

    long signatureId = ++mSubjectId;
    writeKdmType(signatureId, KdmType::Signature());
    writeName(signatureId, name);
    writeContains(callableUnitId, signatureId);

    //Determine return type id
    tree t(TREE_TYPE (TREE_TYPE (functionDecl)));
    tree t2(TYPE_MAIN_VARIANT(t));
    long paramId = writeReturnParameterUnit(t2);
    writeContains(signatureId, paramId);

    //Iterator through argument list
    tree arg(DECL_ARGUMENTS (functionDecl));
    tree argType(TYPE_ARG_TYPES (TREE_TYPE (functionDecl)));
    while (argType && (argType != void_list_node))
    {
        long refId = writeParameterUnit(arg);
        writeContains(signatureId, refId);
        if (arg)
        {
            arg = TREE_CHAIN (arg);
        }
        argType = TREE_CHAIN (argType);
    }
}

void KdmTripleWriter::writeTripleKdmHeader()
{
    *mKdmSink << "KDM_Triple:" << KdmTripleWriter::KdmTripleVersion << "\n";
}

void KdmTripleWriter::writeDefaultKdmModelElements()
{
    writeTriple(SubjectId_Segment, KdmPredicate::KdmType(), KdmType::Segment());
    writeTriple(SubjectId_Segment, KdmPredicate::Uid(), "0");
    writeTriple(SubjectId_Segment, KdmPredicate::LinkId(), "root");
    writeTriple(SubjectId_CodeModel, KdmPredicate::KdmType(), KdmType::CodeModel());
    writeTriple(SubjectId_CodeModel, KdmPredicate::Name(), KdmType::CodeModel());
    writeTriple(SubjectId_CodeModel, KdmPredicate::Uid(), "1");
    writeTriple(SubjectId_CodeModel, KdmPredicate::LinkId(), KdmType::CodeModel());
    writeTriple(SubjectId_Segment, KdmPredicate::Contains(), SubjectId_CodeModel);
    writeTriple(SubjectId_WorkbenchExtensionFamily, KdmPredicate::KdmType(), KdmType::ExtensionFamily());
    writeTriple(SubjectId_WorkbenchExtensionFamily, KdmPredicate::Name(), "__WORKBENCH__");
    writeTriple(SubjectId_WorkbenchExtensionFamily, KdmPredicate::LinkId(), "__WORKBENCH__");
    writeTriple(SubjectId_Segment, KdmPredicate::Contains(), SubjectId_WorkbenchExtensionFamily);
    writeTriple(SubjectId_HiddenStereoType, KdmPredicate::KdmType(), KdmType::StereoType());
    writeTriple(SubjectId_HiddenStereoType, KdmPredicate::Name(), "__HIDDEN__");
    writeTriple(SubjectId_HiddenStereoType, KdmPredicate::LinkId(), "__HIDDEN__");
    writeTriple(SubjectId_WorkbenchExtensionFamily, KdmPredicate::Contains(), SubjectId_HiddenStereoType);
    writeTriple(SubjectId_CodeAssembly, KdmPredicate::KdmType(), KdmType::CodeAssembly());
    writeTriple(SubjectId_CodeAssembly, KdmPredicate::Name(), ":code");
    writeTriple(SubjectId_CodeAssembly, KdmPredicate::Uid(), "2");
    writeTriple(SubjectId_CodeAssembly, KdmPredicate::LinkId(), ":code");
    writeTriple(SubjectId_CodeModel, KdmPredicate::Contains(), SubjectId_CodeAssembly);
    writeTriple(SubjectId_PrimitiveSharedUnit, KdmPredicate::KdmType(), KdmType::SharedUnit());
    writeTriple(SubjectId_PrimitiveSharedUnit, KdmPredicate::Name(), ":primitive");
    writeTriple(SubjectId_PrimitiveSharedUnit, KdmPredicate::Uid(), "3");
    writeTriple(SubjectId_PrimitiveSharedUnit, KdmPredicate::LinkId(), ":primitive");
    writeTriple(SubjectId_CodeAssembly, KdmPredicate::Contains(), SubjectId_PrimitiveSharedUnit);
    writeTriple(SubjectId_DerivedSharedUnit, KdmPredicate::KdmType(), KdmType::SharedUnit());
    writeTriple(SubjectId_DerivedSharedUnit, KdmPredicate::Name(), ":derived");
    writeTriple(SubjectId_DerivedSharedUnit, KdmPredicate::Uid(), "4");
    writeTriple(SubjectId_DerivedSharedUnit, KdmPredicate::LinkId(), ":derived");
    writeTriple(SubjectId_CodeAssembly, KdmPredicate::Contains(), SubjectId_DerivedSharedUnit);
    writeTriple(SubjectId_ClassSharedUnit, KdmPredicate::KdmType(), KdmType::SharedUnit());
    writeTriple(SubjectId_ClassSharedUnit, KdmPredicate::Name(), ":class");
    writeTriple(SubjectId_ClassSharedUnit, KdmPredicate::Uid(), "5");
    writeTriple(SubjectId_ClassSharedUnit, KdmPredicate::LinkId(), ":class");
    writeTriple(SubjectId_CodeAssembly, KdmPredicate::Contains(), SubjectId_ClassSharedUnit);
    writeTriple(SubjectId_InventoryModel, KdmPredicate::KdmType(), KdmType::InventoryModel());
    writeTriple(SubjectId_InventoryModel, KdmPredicate::Name(), KdmType::InventoryModel());
    writeTriple(SubjectId_InventoryModel, KdmPredicate::LinkId(), KdmType::InventoryModel());
    writeTriple(SubjectId_Segment, KdmPredicate::Contains(), SubjectId_InventoryModel);

    writeKdmType(SubjectId_CompilationUnit, KdmType::CompilationUnit());
    writeName(SubjectId_CompilationUnit, mCompilationFile.filename());
    writeTriple(SubjectId_CompilationUnit, KdmPredicate::LinkId(), mCompilationFile.string());
    writeTriple(SubjectId_CodeAssembly, KdmPredicate::Contains(), SubjectId_CompilationUnit);

}

void KdmTripleWriter::writeKdmType(long const subject, KdmType const & object)
{
    writeTriple(subject, KdmPredicate::KdmType(), object);
}

void KdmTripleWriter::writeName(long const subject, std::string const & name)
{
    writeTriple(subject, KdmPredicate::Name(), name);
}

void KdmTripleWriter::writeContains(long const parent, long const child)
{
    writeTriple(parent, KdmPredicate::Contains(), child);
}
void KdmTripleWriter::writeLinkId(long const subject, std::string const & name)
{
    writeTriple(subject, KdmPredicate::LinkId(), name);
}

void KdmTripleWriter::writeSourceFile(boost::filesystem::path const & file)
{
    writeKdmType(++mSubjectId, KdmType::SourceFile());
    writeName(mSubjectId, file.filename());
    writeTriple(mSubjectId, KdmPredicate::Path(), file.string());
    writeTriple(mSubjectId, KdmPredicate::LinkId(), file.string());
    writeTriple(SubjectId_InventoryModel, KdmPredicate::Contains(), mSubjectId);
}

void KdmTripleWriter::writeCompilationUnit(boost::filesystem::path const & file)
{
    writeKdmType(++mSubjectId, KdmType::CompilationUnit());
    writeName(mSubjectId, file.filename());
    writeTriple(mSubjectId, KdmPredicate::LinkId(), file.string());
    writeTriple(SubjectId_CodeAssembly, KdmPredicate::Contains(), mSubjectId);
}

long KdmTripleWriter::writeReturnParameterUnit(tree param)
{
    long ref = findOrAddReferencedNode(param);
    writeKdmType(++mSubjectId, KdmType::ParameterUnit());
    writeTriple(mSubjectId, KdmPredicate::Name(), "__RESULT__");
    writeTriple(mSubjectId, KdmPredicate::Type(), ref);
    return mSubjectId;
}

long KdmTripleWriter::writeParameterUnit(tree param)
{
    //    if (!DECL_ARTIFICIAL (param))
    //    {
    long parameterUnitId(++mSubjectId);
    writeKdmType(parameterUnitId, KdmType::ParameterUnit());
    tree type(TYPE_MAIN_VARIANT(TREE_TYPE(param)));
    long ref = findOrAddReferencedNode(type);

    tree id(DECL_NAME (param));
    std::string name(id ? IDENTIFIER_POINTER (id) : "<unnamed>");
    writeName(parameterUnitId, name);

    //long paramSubjectId = findOrAddReferencedNode(type);

    writeTriple(parameterUnitId, KdmPredicate::Type(), ref);
    return parameterUnitId;
    //    }
}

long KdmTripleWriter::findOrAddReferencedNode(tree node)
{
    long retValue(-1);
    std::pair<AstNodeReferenceMap::iterator, bool> result = referencedNodes.insert(std::make_pair(node, mSubjectId + 1));
    if (result.second)
    {
        retValue = ++mSubjectId;
        tree treeType(TREE_TYPE(node));
        if (treeType)
        {
            tree t2(TYPE_MAIN_VARIANT(treeType));
            findOrAddReferencedNode(t2);
        }
    }
    else
    {
        retValue = result.first->second;
    }
    return retValue;
}

void KdmTripleWriter::writePrimitiveType(tree type)
{
    long typeSubjectId = findOrAddReferencedNode(type);
    writeKdmType(typeSubjectId, KdmType::PrimitiveType());

    //Some fundamental types do not have names...
    tree typeName(TYPE_NAME (type));
    tree treeName = (TREE_CODE(typeName) == IDENTIFIER_NODE) ? typeName : DECL_NAME (typeName);
    std::string name(treeName ? IDENTIFIER_POINTER (treeName) : "<unnamed>");

    writeName(typeSubjectId, name);
}

void KdmTripleWriter::writePointerType(tree pointerType)
{
    long pointerSubjectId = findOrAddReferencedNode(pointerType);
    writeKdmType(pointerSubjectId, KdmType::PointerType());
    writeName(pointerSubjectId, "PointerType");

    tree treeType(TREE_TYPE(pointerType));
    tree t2(TYPE_MAIN_VARIANT(treeType));
    long pointerTypeSubjectId = findOrAddReferencedNode(t2);
    writeTriple(pointerSubjectId, KdmPredicate::Type(), pointerTypeSubjectId);

}

void KdmTripleWriter::writeRecordType(tree recordType)
{
    //    if (DECL_ARTIFICIAL (recordType))
    //    {
    //        std::cerr << "artificial" << std::endl;
    //    }

    //    std::cerr << "writeRecordType: " << typeNameString(recordType) << std::endl;
    recordType = TYPE_MAIN_VARIANT (recordType);


    tree name = TYPE_NAME (recordType);
    if (name && TREE_CODE (name) == TYPE_DECL)
    {
        name = DECL_NAME (name);
    }
    if (!name || ANON_AGGRNAME_P (name))
    {
        std::cerr << "Anonymous Struct :" << locationString(locationOf(recordType)) << std::endl;
    }
    else
    {
        std::cerr << typeNameString(recordType) << " " << locationString(locationOf(recordType)) << std::endl;
    }


//    if (!TYPE_ANONYMOUS_P (recordType))
//    {
//        tree decl(TYPE_NAME(recordType));
//        if (decl)
//        {
//
//            std::cerr << typeNameString(recordType) << " " << locationString(locationOf(recordType)) << std::endl;
//            //            tree id (DECL_NAME (TYPE_NAME(recordType)));
//            //            const char* name (IDENTIFIER_POINTER (decl));
//            //std::cerr << "struct " << name << " at \n";// << DECL_SOURCE_FILE (recordType) << ":" << DECL_SOURCE_LINE (recordType) << std::endl;
//
//        }
//        else
//        {
//            std::cerr << "no name???? =======================" << locationString(locationOf(recordType)) << std::endl;
//        }
//        //        tree decl(TYPE_NAME(recordType));
//    }
//    else
//    {
//        std::cerr << "anonymous struct" << std::endl;
//    }
    ////    if (RECORD_OR_UNION_CODE_P(TREE_CODE(recordType)))
    ////    {
    ////        if (TREE_CODE(recordType) == RECORD_TYPE)
    ////        {
    ////            if (CLASSTYPE_DECLARED_CLASS (recordType))
    ////            {
    ////                //we have a class
    ////            }
    ////            else
    ////            {
    ////                //we have a struct
    ////                long structId = ++mSubjectId;
    ////                writeKdmType(structId, KdmType::RecordType());
    ////
    ////                if (!TYPE_ANONYMOUS_P (recordType))
    ////                {
    ////                    tree typeName(TYPE_NAME (recordType));
    ////                    tree id(DECL_NAME (typeName));
    ////                    std::string name(id ? IDENTIFIER_POINTER (id) : "<unnamed>");
    ////                    writeName(structId, name);
    ////    //                xml_print_name_attribute(xdi, DECL_NAME (TYPE_NAME (rt)));
    ////                }
    ////
    ////            }
    ////        }
    ////    }
}

//void KdmTripleWriter::writeDirectory()
//{
//
//}
//
//void KdmTripleWriter::writeDirectoryStructure()
//{
//    for(PathSet::const_iterator i= mPaths.start(), e=mPaths.end(); i!=e ;++i)
//    {
//        writeDirectory((*i));
//    }
//}
//
//void KdmTripleWriter::addPath(boost::filesystem::path const & newPath)
//{
//    for (boost::filesystem::path::const_iterator i= newPath.begin(), e=newPath.end(); i!=e; ++i)
//    {
//       mPaths.insert(*i);
//    }
//}


} // namespace kdmtriplewriter

} // namespace gcckdm
