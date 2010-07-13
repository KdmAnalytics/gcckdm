#ifndef GCCKDM_KDMTRIPLEWRITER_PATH_HASH_HH
#define GCCKDM_KDMTRIPLEWRITER_PATH_HASH_HH

#include <string>
#include <boost/filesystem/path.hpp>
#include <boost/lexical_cast.hpp>
#include "gcckdm/GccAstListener.hh"

namespace std {

namespace tr1 {

template <>
struct hash<gcckdm::GccAstListener::Path>
{
    size_t operator()(gcckdm::GccAstListener::Path const & v) const
    {
       hash<gcckdm::GccAstListener::Path::string_type> h;
       return h(v.string());
    }
};

}  // namespace tr1

}  // namespace std



#endif
