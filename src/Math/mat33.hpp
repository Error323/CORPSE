#ifndef PFFG_MAT33_HDR
#define PFFG_MAT33_HDR

template<typename T> struct vec3;
template<typename T> struct mat33 {
	mat33<T>() {
		m[0] = T(1); m[1] = T(0); m[2] = T(0);
		m[3] = T(0); m[4] = T(1); m[5] = T(0);
		m[6] = T(0); m[7] = T(0); m[8] = T(1);
	}
	mat33<T>(
		T xvx, T xvy, T xvz,
		T yvx, T yvy, T yvz,
		T zvx, T zvy, T zvz) {
		m[0] = xvx; m[1] = xvy; m[2] = xvz;
		m[3] = yvx; m[4] = yvy; m[5] = yvz;
		m[6] = zvx; m[7] = zvy; m[8] = zvz;
	}
	mat33<T>(const vec3<T>& xv, const vec3<T>& yv, const vec3<T>& zv) {
		m[0] = xv.x; m[1] = xv.y; m[2] = xv.z;
		m[3] = yv.x; m[4] = yv.y; m[5] = yv.z;
		m[6] = zv.x; m[7] = zv.y; m[8] = zv.z;
	}



	inline mat33<T> operator * (T s) const {
		return mat33<T>(
			m[0] * s, m[1] * s, m[2] * s,
			m[3] * s, m[4] * s, m[5] * s,
			m[6] * s, m[7] * s, m[8] * s
		);
	}

	// M * v = v' (with v a col-vector)
	inline vec3<T> operator * (const vec3<T>& v) const {
		return vec3<T>(
			m[0] * v.x  +  m[3] * v.y  +  m[6] * v.z,
			m[1] * v.x  +  m[4] * v.y  +  m[7] * v.z,
			m[2] * v.x  +  m[5] * v.y  +  m[8] * v.z
		);
	}

	inline mat33<T> operator - (const mat33<T>& n) const {
		// return mat33<T>(x - m.x, y - m.y, z - m.z);
		return mat33<T>(
			m[0] - n.m[0], m[1] - n.m[1], m[2] - n.m[2],
			m[3] - n.m[3], m[4] - n.m[4], m[5] - n.m[5],
			m[6] - n.m[6], m[7] - n.m[7], m[8] - n.m[8]
		);
	}

	inline mat33<T>& operator += (const mat33<T>& n) {
		m[0] += n.m[0]; m[1] += n.m[1]; m[2] += n.m[2];
		m[3] += n.m[3]; m[4] += n.m[4]; m[5] += n.m[5];
		m[6] += n.m[6]; m[7] += n.m[7]; m[8] += n.m[8];
		return *this;
	}



	const char* str() const;

	// OpenGL ordered (ie. column-major)
	// note: when using an array representation,
	// we can not (pre-)initialize the elements
	// in the constructors, but separate floats
	// or vec3's are ugly or require one extra
	// #include
	T m[9];
};

const mat33<float> I33FMAT = mat33<float>();

#endif
