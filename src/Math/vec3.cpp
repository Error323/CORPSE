#include <cstdlib>
#include <string>
#include <sstream>

#include "./vec3.hpp"

// specialization for floats
template<> vec3<float>& vec3<float>::randomize() {
	x = random();
	y = random();
	z = random();
	return (inorm());
}

template<> std::string vec3<float>::str() const {
	std::string s;
	std::stringstream ss;
		ss << "<";
		ss << x << ", ";
		ss << y << ", ";
		ss << z;
		ss << ">";
	s = ss.str();
	return s;
}
