#ifndef PFFG_VEC4_HDR
#define PFFG_VEC4_HDR

#include "./vec3.hpp"

template<typename T> struct vec4: public vec3<T> {
	vec4<T>():                       vec3<T>(             ), w(T(0)) {}
	vec4<T>(T _x, T _y, T _z, T _w): vec3<T>(_x,   _y,  _z), w( _w)  {}
	vec4<T>(const vec3<T>& v, T _w): vec3<T>(v            ), w( _w)  {}
	vec4<T>(const vec4<T>& v):       vec3<T>(v.x, v.y, v.z), w(v.w)  {}

	inline vec4<T>& operator = (const vec4<T>& v) {
		// for some reason the explicit this-pointer
		// dereferences are necessary or {x, y, z}
		// are seen as "not declared in this scope"
		this->x = v.x;
		this->y = v.y;
		this->z = v.z;
		this->w = v.w;
		return *this;
	}
	inline vec4<T> operator * (const T s) const { return vec4<T>(this->x * s, this->y * s, this->z * s, this->w * s); }
	inline vec4<T> operator / (const T s) const { return vec4<T>(this->x / s, this->y / s, this->z / s, this->w / s); }

	// allows implicit conversion to const T*
	inline operator const T* () const { return &(this->x); }

	T w;
};

#endif
