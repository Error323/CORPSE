#include <sstream>
#include "./mat33.hpp"

// specialization for floats
template<> const char* mat33<float>::str() const {
	static std::string s;
	std::stringstream ss;
		ss << m[0] << " " << m[3] << " " << m[6] << std::endl;
		ss << m[1] << " " << m[4] << " " << m[7] << std::endl;
		ss << m[2] << " " << m[5] << " " << m[8] << std::endl;
	s = ss.str();

	return (s.c_str());
}
