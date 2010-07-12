#ifndef GCCKDM_KDMTRIPLEWRITER_PATH_HASH_HH
#define GCCKDM_KDMTRIPLEWRITER_PATH_HASH_HH

#include <boost/lexical_cast.hpp>

namespace std {

namespace tr1 {

template <>
struct hash<boost::filesystem::path>
{
    size_t operator()(boost::filesystem::path const & v) const
    {
       hash<std::string> h;
       return h(v.string());
    }
};

template <>
struct hash<expanded_location>
{
	size_t operator() (expanded_location const & v) const
	{
		hash<std::string> h;
		return h(std::string(v.file) + boost::lexical_cast<std::string>(v.line));
	}
};

}  // namespace tr1

}  // namespace std

struct location_equal
{
	bool operator()(expanded_location const & rhs, expanded_location const & lhs) const
	{
		return (std::string(rhs.file) == std::string(lhs.file)) && (rhs.line == lhs.line);
	}
};


#endif
