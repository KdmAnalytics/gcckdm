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

void KdmTripleWriter::start(boost::filesystem::path const & file)
{
    writeTripleKdmHeader();
    writeDefaultKdmModelElements();
    writeSourceFile(file);
}

void KdmTripleWriter::processAstNode(tree ast)
{
    assert(DECL_P(ast));
    tree type(TREE_TYPE(ast));
    int declCode(TREE_CODE(ast));

    switch (declCode)
    {
        case FUNCTION_DECL:
        {
            processFunctionDeclaration(ast);
            break;
        }
        default:
        {
            std::cerr << "unsupported declaration " << tree_code_name[declCode] << std::endl;
        }
    }
}

void KdmTripleWriter::finish()
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

void KdmTripleWriter::processFunctionDeclaration(tree functionDecl)
{
    writeCallableUnit(functionDecl);
}


void KdmTripleWriter::writeCallableUnit(tree functionDecl)
{
    //    int tc(TREE_CODE(functionDecl));
    tree id(DECL_NAME (functionDecl));
    std::string name(id ? IDENTIFIER_POINTER (id) : "<unnamed>");
    //    tree type(TREE_TYPE(functionDecl));
    //    cerr << tree_code_name[tc] << " " << getScopeString(decl) << "::" << name << " type " << tree_code_name[TREE_CODE(type)] << " at "
    //            << DECL_SOURCE_FILE (decl) << ":" << DECL_SOURCE_LINE (decl) << endl;

    long callableUnitId = ++mSubjectId;
    writeKdmType(callableUnitId, KdmType::CallableUnit());
    writeName(callableUnitId, name);
    writeLinkId(callableUnitId, name);
    writeContains(SubjectId_ClassSharedUnit, callableUnitId);

    long signatureId = ++mSubjectId;
    writeKdmType(signatureId, KdmType::Signature());
    writeName(signatureId, name);
    writeContains(callableUnitId, signatureId);

    //Determine return type id
    tree t(TREE_TYPE (TREE_TYPE (functionDecl)));
    tree t2(TYPE_MAIN_VARIANT(t));


    //Iterator through argument list
    tree arg(DECL_ARGUMENTS (functionDecl));
    tree argType(TYPE_ARG_TYPES (TREE_TYPE (functionDecl)));
    while (argType && (argType != void_list_node))
    {
        writeParameterUnit(arg);
        if (arg)
            arg = TREE_CHAIN (arg);
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

void KdmTripleWriter::writeParameterUnit(tree param)
{
    //Skip any compiler-generated parameters
//    if (param && DECL_ARTIFICIAL (param))
//        return;

    tree id(DECL_NAME (param));
    std::string name(id ? IDENTIFIER_POINTER (id) : "<unnamed>");

    writeKdmType(++mSubjectId, KdmType::ParameterUnit());
    writeName(mSubjectId,name );
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
