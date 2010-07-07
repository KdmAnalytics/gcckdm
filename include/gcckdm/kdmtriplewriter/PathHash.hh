#ifndef GCCKDM_KDMTRIPLEWRITER_PATH_HASH_HH
#define GCCKDM_KDMTRIPLEWRITER_PATH_HASH_HH


namespace std {

namespace tr1 {

template <>
struct hash<boost::filesystem::path>
{
    size_t operator()(const boost::filesystem::path& v) const
    {
       hash<std::string> h;
       return h(v.string());
    }
};


}  // namespace tr1

}  // namespace std


#endif
