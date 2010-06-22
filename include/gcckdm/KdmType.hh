/*
 * KdmType.hh
 *
 *  Created on: Jun 22, 2010
 *      Author: kgirard
 */

#ifndef KDMTYPE_HH_
#define KDMTYPE_HH_

#include <string>
#include <iosfwd>

namespace gcckdm {

class KdmType {
private:
    enum
    {
        //Kdm Group
        KdmType_Segment,
        KdmType_ExtensionFamily,
        KdmType_StereoType,

        //Code Group
        KdmType_CodeModel,
        KdmType_CodeAssembly,
        KdmType_SharedUnit,
        KdmType_StoreableUnit,
        KdmType_Value,

        //Source Group
        KdmType_InventoryModel,
        KdmType_SourceFile,


        //Action Group
        KdmType_ActionElement,
        KdmType_Addresses,
        KdmType_Writes,
        KdmType_Reads,
        KdmType_BlockUnit,
        KdmType_Flow,
    };

public:
    //Kdm Group
    static const KdmType Segment() { return KdmType(KdmType_Segment,"kdm/Segment");}
    static const KdmType ExtensionFamily() { return KdmType(KdmType_ExtensionFamily, "kdm/ExtensionFamily");}
    static const KdmType StereoType() { return KdmType(KdmType_StereoType, "kdm/StereoType");}

    //Code Group
    static const KdmType CodeModel() { return KdmType(KdmType_CodeModel, "code/CodeModel");}
    static const KdmType CodeAssembly() { return KdmType(KdmType_CodeAssembly, "code/CodeAssembly");}
    static const KdmType SharedUnit() { return KdmType(KdmType_SharedUnit, "code/SharedUnit");}
    static const KdmType StoreableUnit() { return KdmType(KdmType_StoreableUnit, "code/StoreableUnit");}
    static const KdmType Value() { return KdmType(KdmType_Value, "code/Value");}
    //static const KdmType () { return KdmType(KdmType_, "code/");}

    //Source Group
    static const KdmType InventoryModel() { return KdmType(KdmType_InventoryModel, "source/InventoryModel");}
    static const KdmType SourceFile() { return KdmType(KdmType_SourceFile, "source/SourceFile");}

    //Action Group
    static const KdmType ActionElement() { return KdmType(KdmType_ActionElement, "action/ActionElement");}
    static const KdmType Addresses() { return KdmType(KdmType_Addresses, "action/Addresses");}
    static const KdmType Writes() { return KdmType(KdmType_Writes, "action/Writes");}
    static const KdmType Reads() { return KdmType(KdmType_Reads, "action/Reads");}
    static const KdmType BlockUnit() { return KdmType(KdmType_BlockUnit, "action/BlockUnit");}
    static const KdmType Flow() { return KdmType(KdmType_Flow, "action/Flow");}
    //static const KdmType () { return KdmType(KdmType_, "action/");}
    //static const KdmType () { return KdmType(KdmType_, "action/");}

    int const id() const { return mId; }
    operator int() const { return mId; }
    std::string const & name() const { return mName; }

private:
    KdmType(int const & id, std::string name) : mId(id), mName(name){}

    int mId;
    std::string mName;
};

std::ostream & operator<< (std::ostream & sink, KdmType const & pred);

}  // namespace gcckdm

#endif /* KDMTYPE_HH_ */
