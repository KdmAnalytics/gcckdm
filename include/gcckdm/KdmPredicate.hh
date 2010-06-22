/*
 * KdmPredicate.hh
 *
 *  Created on: Jun 22, 2010
 *      Author: kgirard
 */

#ifndef KDMPREDICATE_HH_
#define KDMPREDICATE_HH_

#include <string>
#include <iosfwd>

namespace gcckdm {

class KdmPredicate
{
private:
    enum
    {
        KdmPredicate_KdmType,
        KdmPredicate_Contains,
        KdmPredicate_Name,
        KdmPredicate_Kind,
        KdmPredicate_From,
        KdmPredicate_To,
        KdmPredicate_Path,
        KdmPredicate_LinkId,
        KdmPredicate_Uid,
        KdmPredicate_LastUid,
   };

public:
    static const KdmPredicate KdmType() { return KdmPredicate(KdmPredicate_KdmType,"kdmtype");}
    static const KdmPredicate Contains() { return KdmPredicate(KdmPredicate_Contains, "contains");}
    static const KdmPredicate Name() { return KdmPredicate(KdmPredicate_Name, "name");}
    static const KdmPredicate Kind() { return KdmPredicate(KdmPredicate_Kind, "kind");}
    static const KdmPredicate From() { return KdmPredicate(KdmPredicate_From, "from");}
    static const KdmPredicate To() { return KdmPredicate(KdmPredicate_To, "to");}
    static const KdmPredicate Path() { return KdmPredicate(KdmPredicate_Path, "path");}

    static const KdmPredicate LinkId() { return KdmPredicate(KdmPredicate_LinkId, "link::id");}
    static const KdmPredicate Uid() { return KdmPredicate(KdmPredicate_Uid, "UID");}
    static const KdmPredicate LastUid() { return KdmPredicate(KdmPredicate_LastUid, "LastUID");}


    int const id() const { return mId; }
    operator int() const { return mId; }
    std::string const & name() const { return mName; }

private:
    KdmPredicate(int const & id, std::string name) : mId(id), mName(name){}

    int mId;
    std::string mName;
};

std::ostream & operator<< (std::ostream & sink, KdmPredicate const & pred);


}  // namespace gcckdm

#endif /* KDMPREDICATE_HH_ */
