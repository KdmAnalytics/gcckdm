/*
 * KdmKind.hh
 *
 *  Created on: Jul 16, 2010
 *      Author: kgirard
 */

#ifndef GCCKDM_KDMKIND_HH_
#define GCCKDM_KDMKIND_HH_

namespace gcckdm {

class KdmKind {
private:
private:
    enum
    {
        //Kdm Group
        KdmKind_Assign,
        KdmKind_Add,
        KdmKind_Subtract,
        KdmKind_Multiply,
        KdmKind_Divide,
        KdmKind_Negate,
  };

public:
    static const KdmKind Assign() { return KdmKind(KdmKind_Assign,"Assign");}
    static const KdmKind Add() { return KdmKind(KdmKind_Add,"Add");}
    static const KdmKind Subtract() { return KdmKind(KdmKind_Subtract,"Subtract");}
    static const KdmKind Multiply() { return KdmKind(KdmKind_Multiply,"Multiply");}
    static const KdmKind Divide() { return KdmKind(KdmKind_Divide,"Divide");}
    static const KdmKind Negate() { return KdmKind(KdmKind_Negate,"Negate");}

    int const id() const { return mId; }
    operator int() const { return mId; }
    std::string const & name() const { return mName; }

private:
    KdmKind(int const & id, std::string name) : mId(id), mName(name){}

    int mId;
    std::string mName;
};

}  // namespace gcckdm

#endif /* GCCKDM_KDMKIND_HH_ */
