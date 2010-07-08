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

struct location_equal
{
	bool operator()(location_t const rhs, location_t const lhs) const
	{
		expanded_location rhs2 = expand_location(rhs);
		expanded_location lhs2 = expand_location(lhs);
		return (std::string(rhs2.file) == std::string(lhs2.file)) && (rhs2.line == lhs2.line);
	}
};


#endif
