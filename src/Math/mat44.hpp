#ifndef PFFG_MAT44_HDR
#define PFFG_MAT44_HDR

#include <cmath>
#include "./vec3.hpp"

// convert a (row, col) coordinate to a
// 1D array index in column-major order
//
// #define GetIdx1D(i, j) (i + (j * 4))

template<typename T> struct vec3;
template<typename T> struct vec4;
template<typename T> struct mat44 {
	mat44<T>(void) { LoadIdentity(); }
	mat44<T>(const vec3<T>& p, const vec3<T>& x, const vec3<T>& y, const vec3<T>& z) {
		m[ 0] = x.x; m[ 1] = x.y; m[ 2] = x.z; m[ 3] = T(0);
		m[ 4] = y.x; m[ 5] = y.y; m[ 6] = y.z; m[ 7] = T(0);
		m[ 8] = z.x; m[ 9] = z.y; m[10] = z.z; m[11] = T(0);
		m[12] = p.x; m[13] = p.y; m[14] = p.z; m[15] = T(1);
		UpdatePXYZ();
	}
	mat44<T>(const mat44<T>& n) {
		m[ 0] = n[ 0];  m[ 1] = n[ 1];  m[ 2] = n[ 2];  m[ 3] = n[ 3];
		m[ 4] = n[ 4];  m[ 5] = n[ 5];  m[ 6] = n[ 6];  m[ 7] = n[ 7];
		m[ 8] = n[ 8];  m[ 9] = n[ 9];  m[10] = n[10];  m[11] = n[11];
		m[12] = n[12];  m[13] = n[13];  m[14] = n[14];  m[15] = n[15];
		UpdatePXYZ();
	}
	mat44<T>(const T* n) {
		m[ 0] = n[ 0];  m[ 1] = n[ 1];  m[ 2] = n[ 2];  m[ 3] = n[ 3];
		m[ 4] = n[ 4];  m[ 5] = n[ 5];  m[ 6] = n[ 6];  m[ 7] = n[ 7];
		m[ 8] = n[ 8];  m[ 9] = n[ 9];  m[10] = n[10];  m[11] = n[11];
		m[12] = n[12];  m[13] = n[13];  m[14] = n[14];  m[15] = n[15];
		UpdatePXYZ();
	}

	inline mat44<T>& LoadIdentity() {
		m[ 0] = m[ 5] = m[10] = m[15] = T(1);
		m[ 1] = m[ 2] = m[ 3] = T(0);
		m[ 4] = m[ 6] = m[ 7] = T(0);
		m[ 8] = m[ 9] = m[11] = T(0);
		m[12] = m[13] = m[14] = T(0);
		UpdatePXYZ();
		return *this;
	}

	// update the shadow copies
	inline void UpdatePXYZ() {
		 pos.x = m[12];  pos.y = m[13];  pos.z = m[14];
		xdir.x = m[ 0]; xdir.y = m[ 1]; xdir.z = m[ 2];
		ydir.x = m[ 4]; ydir.y = m[ 5]; ydir.z = m[ 6];
		zdir.x = m[ 8]; zdir.y = m[ 9]; zdir.z = m[10];
	}


	inline T operator [] (int i) const {
		return m[i];
	}
	inline void operator = (const mat44<T>& n) {
		for (int i = 0; i < 16; ++i) {
			m[i] = n[i];
		}

		UpdatePXYZ();
	}


	inline void RotateX(float rad) {
		const float sr = sinf(rad);
		const float cr = cosf(rad);

		T a = m[4];
			m[4] = T(cr * a - sr * m[8]);
			m[8] = T(sr * a + cr * m[8]);

		a = m[5];
			m[5] = T(cr * a - sr * m[9]);
			m[9] = T(sr * a + cr * m[9]);

		a = m[6];
			m[ 6] = T(cr * a - sr * m[10]);
			m[10] = T(sr * a + cr * m[10]);

		a = m[7];
			m[ 7] = T(cr * a - sr * m[11]);
			m[11] = T(sr * a + cr * m[11]);

		UpdatePXYZ();
	}
	inline void RotateY(float rad) {
		const float sr = sinf(rad);
		const float cr = cosf(rad);

		T a = m[0];
			m[0] = T( cr * a + sr * m[8]);
			m[8] = T(-sr * a + cr * m[8]);

		a = m[1];
			m[1] = T( cr * a + sr * m[9]);
			m[9] = T(-sr * a + cr * m[9]);

		a = m[2];
			m[ 2] = T( cr * a + sr * m[10]);
			m[10] = T(-sr * a + cr * m[10]);

		a = m[3];
			m[ 3] = T( cr * a + sr * m[11]);
			m[11] = T(-sr * a + cr * m[11]);

		UpdatePXYZ();
	}
	inline void RotateZ(float rad) {
		const float sr = sinf(rad);
		const float cr = cosf(rad);

		T a = m[0];
			m[0] = T(cr * a - sr * m[4]);
			m[4] = T(sr * a + cr * m[4]);

		a = m[1];
			m[1] = T(cr * a - sr * m[5]);
			m[5] = T(sr * a + cr * m[5]);

		a = m[2];
			m[2] = T(cr * a - sr * m[6]);
			m[6] = T(sr * a + cr * m[6]);

		a = m[3];
			m[3] = T(cr * a - sr * m[7]);
			m[7] = T(sr * a + cr * m[7]);

		UpdatePXYZ();
	}

	// rotate <rad> radians around axis <axis>
	inline void Rotate(float rad, vec3<T>& axis) {
		const float sr = sinf(rad);
		const float cr = cosf(rad);

		for (int a = 0; a < 3; ++a) {
			vec3<T> v(m[a * 4], m[a * 4 + 1], m[a * 4 + 2]);

			vec3<T> va(axis * v.dot3D(axis));
			vec3<T> vp(v - va);
			vec3<T> vp2(axis.cross(vp));

			vec3<T> vpnew(vp * cr + vp2 * sr);
			vec3<T> vnew(va + vpnew);

			m[a * 4    ] = vnew.x;
			m[a * 4 + 1] = vnew.y;
			m[a * 4 + 2] = vnew.z;
		}

		UpdatePXYZ();
	}

	inline mat44<T>& Translate(T x, T y, T z) {
		m[12] += (x * m[0] + y * m[4] + z * m[ 8]);
		m[13] += (x * m[1] + y * m[5] + z * m[ 9]);
		m[14] += (x * m[2] + y * m[6] + z * m[10]);
		m[15] += (x * m[3] + y * m[7] + z * m[11]);

		pos.x = m[12];
		pos.y = m[13];
		pos.z = m[14];

		return *this;
	}
	inline mat44<T>& Translate(const vec3<T>& pos) {
		return (Translate(pos.x, pos.y, pos.z));
	}


	inline mat44<T> Mul(const mat44<T>& n) const {
		mat44<T> r;

		for (int i = 0; i < 16; i += 4) {
			// get column (i % 4)
			const T m20 = n[i    ];
			const T m21 = n[i + 1];
			const T m22 = n[i + 2];
			const T m23 = n[i + 3];

			// set column (i % 4)
			r.m[i    ] = m[0] * m20 + m[4] * m21 + m[ 8] * m22 + m[12] * m23;
			r.m[i + 1] = m[1] * m20 + m[5] * m21 + m[ 9] * m22 + m[13] * m23;
			r.m[i + 2] = m[2] * m20 + m[6] * m21 + m[10] * m22 + m[14] * m23;
			r.m[i + 3] = m[3] * m20 + m[7] * m21 + m[11] * m22 + m[15] * m23;
		}

		r.UpdatePXYZ();
		return r;
	}

	inline mat44<T> Mul(const T* n) const {
		mat44<T> r;

		for (int i = 0; i < 16; i += 4) {
			const T m20 = n[i    ];
			const T m21 = n[i + 1];
			const T m22 = n[i + 2];
			const T m23 = n[i + 3];

			r.m[i    ] = m[0] * m20 + m[4] * m21 + m[ 8] * m22 + m[12] * m23;
			r.m[i + 1] = m[1] * m20 + m[5] * m21 + m[ 9] * m22 + m[13] * m23;
			r.m[i + 2] = m[2] * m20 + m[6] * m21 + m[10] * m22 + m[14] * m23;
			r.m[i + 3] = m[3] * m20 + m[7] * m21 + m[11] * m22 + m[15] * m23;
		}

		r.UpdatePXYZ();
		return r;
	}


	inline mat44<T>& ITranspose() {
		T t = T(0);

		// first row <==> first col
		t = m[ 1]; m[ 1] = m[ 4]; m[ 4] = t;
		t = m[ 2]; m[ 2] = m[ 8]; m[ 8] = t;
		t = m[ 3]; m[ 3] = m[12]; m[12] = t;

		// second row <==> second col
		t = m[ 6]; m[ 6] = m[ 9]; m[ 9] = t;
		t = m[ 7]; m[ 7] = m[13]; m[13] = t;

		// third row <==> third col
		t = m[11]; m[11] = m[14]; m[14] = t;

		UpdatePXYZ();
		return *this;
	}
	inline mat44<T> Transpose() const {
		mat44<T> n(*this);

		// first row <==> first col
		n.m[ 1] = m[ 4]; n.m[ 4] = m[ 1];
		n.m[ 2] = m[ 8]; n.m[ 8] = m[ 2];
		n.m[ 3] = m[ 3]; n.m[12] = m[12];

		// second row <==> second col
		n.m[ 6] = m[ 9]; n.m[ 9] = m[ 6];
		n.m[ 7] = m[13]; n.m[13] = m[ 7];

		// third row <==> third col
		n.m[11] = m[14]; n.m[14] = m[11];

		n.UpdatePXYZ();
		return n;
	}

	// note: assumes this matrix only
	// does translation and rotation;
	// m.Mul(m.Invert()) is identity
	inline mat44<T> Invert() const {
		mat44<T> mInv(*this);

		// transpose the rotation
		mInv.m[1] = m[4]; mInv.m[4] = m[1];
		mInv.m[2] = m[8]; mInv.m[8] = m[2];
		mInv.m[6] = m[9]; mInv.m[9] = m[6];

		// get the inverse translation
		const vec3<T> t(-m[12], -m[13], -m[14]);

		// do the actual inversion
		mInv.m[12] = t.x * mInv[0] + t.y * mInv[4] + t.z * mInv[ 8];
		mInv.m[13] = t.x * mInv[1] + t.y * mInv[5] + t.z * mInv[ 9];
		mInv.m[14] = t.x * mInv[2] + t.y * mInv[6] + t.z * mInv[10];

		mInv.UpdatePXYZ();
		return mInv;
	}
	// note: assumes this matrix only
	// does translation and rotation
	inline mat44<T>& IInvert() {
		// transpose the rotation
		T t = T(0);
		t = m[1]; m[1] = m[4]; m[4] = t;
		t = m[2]; m[2] = m[8]; m[8] = t;
		t = m[6]; m[6] = m[9]; m[9] = t;

		// get the inverse translation
		vec3<T> tr(-m[12], -m[13], -m[14]);

		// do the actual inversion
		m[12] = tr.x * m[0] + tr.y * m[4] + tr.z * m[ 8];
		m[13] = tr.x * m[1] + tr.y * m[5] + tr.z * m[ 9];
		m[14] = tr.x * m[2] + tr.y * m[6] + tr.z * m[10];

		UpdatePXYZ();
		return *this;
	}


	inline vec3<T> Mul(const vec3<T>& v) const {
		// technically illegal to apply a mat44 to a vec3,
		// but we only want the rotation part of the mat
		// (upper-left 3x3 elements)
		const T x = v.x * m[0] + v.y * m[4] + v.z * m[ 8] + m[12];
		const T y = v.x * m[1] + v.y * m[5] + v.z * m[ 9] + m[13];
		const T z = v.x * m[2] + v.y * m[6] + v.z * m[10] + m[14];
		return vec3<T>(x, y, z);
	}
	inline vec4<T> Mul(const vec4<T>& v) const {
		const T x = v.x * m[0] + v.y * m[4] + v.z * m[ 8] + v.w * m[12];
		const T y = v.x * m[1] + v.y * m[5] + v.z * m[ 9] + v.w * m[13];
		const T z = v.x * m[2] + v.y * m[6] + v.z * m[10] + v.w * m[14];
		const T w = v.x * m[3] + v.y * m[7] + v.z * m[11] + v.w * m[15];
		return vec4<T>(x, y, z, w);
	}

	/*
	inline vec3<T> GetDir(int i) const {
		#ifdef PFFG_DEBUG
		assert(i >= 0 && i <= 2);
		#endif
		return (vec3<T>(m[(i * 4) + 0], m[(i * 4) + 1], m[(i * 4) + 2]));
	}
	*/
	inline const vec3<T>& GetPos()  const { return pos;  }
	inline const vec3<T>& GetXDir() const { return xdir; }
	inline const vec3<T>& GetYDir() const { return ydir; }
	inline const vec3<T>& GetZDir() const { return zdir; }

	inline void SetPos(const vec3<T>& p) {
		m[12] = p.x;
		m[13] = p.y;
		m[14] = p.z;
		pos   = p;
	}

	inline void SetXDir(const vec3<T>& x) {
		m[ 0] = x.x;
		m[ 1] = x.y;
		m[ 2] = x.z;
		xdir  = x;
	}
	inline void SetYDir(const vec3<T>& y) {
		m[ 4] = y.x;
		m[ 5] = y.y;
		m[ 6] = y.z;
		ydir  = y;
	}
	inline void SetZDir(const vec3<T>& z) {
		m[ 8] = z.x;
		m[ 9] = z.y;
		m[10] = z.z;
		zdir  = z;
	}

	// sets Y and recalculates X and Z
	inline void SetYDirXZ(const vec3<T>& y) {
		SetYDir(y);

		xdir = (zdir.cross(ydir)).inorm();
		zdir = (ydir.cross(xdir)).inorm();

		m[ 0] = xdir.x;
		m[ 1] = xdir.y;
		m[ 2] = xdir.z;

		m[ 8] = zdir.x;
		m[ 9] = zdir.y;
		m[10] = zdir.z;
	}

	const char* str() const;

	private: vec3<T> pos;  // shadow copy of m[12 .. 16]
	private: vec3<T> xdir; // shadow copy of m[ 0 ..  4]
	private: vec3<T> ydir; // shadow copy of m[ 4 ..  8]
	private: vec3<T> zdir; // shadow copy of m[ 8 .. 12]

	// OpenGL ordered (ie. column-major)
	public: T m[16];
};

#endif
