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
    };

public:
    static const KdmKind Assign() { return KdmKind(KdmKind_Assign,"Assign");}

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
