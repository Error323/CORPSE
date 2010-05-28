#include <string>
#include <sstream>

#include "./mat44.hpp"

/*
// int Pivot(int, T[4][4], T* [4]) const;
template<typename T>
int mat44<T>::Pivot(int row, T mrows[4][4], T* rmptrs[4]) const {
	const int nrows = 4;
	int       k     = row;
	T         max   = T(-1);
	T*        rptr  = 0x0;

	for (int i = row; i < nrows; i++) {
		// NOTE: [i][row] ????
		T t = fabsf(mrows[i][row]);

		if (t > max && t != T(0)) {
			max = t;
			k = i;
		}
	}

	// NOTE: [k][row] ????
	if (mrows[k][row] == T(0)) {
		return -1;
	}

	if (k != row) {
		// swap rows <k> and <row>
		rptr        = rmptrs[k  ];
		rmptrs[k  ] = rmptrs[row];
		rmptrs[row] = rptr;
		return k;
	}

	return 0;
}

// special-case implementation of the generic
// inversion algorithm for our 4x4 matrices
// (which are always square and non-singular)
//
// mat44<T> Inv() const;
template<typename T>
mat44<T> mat44<T>::Inv() const {
	mat44<T> mInv;
	const int nrows = 4;

	// copy the current mat
	T mrows[nrows][nrows] = {
		{m[0], m[4], m[ 8], m[12]}, // 1st row
		{m[1], m[5], m[ 9], m[13]}, // 2nd row
		{m[2], m[6], m[10], m[14]}, // 3rd row
		{m[3], m[7], m[11], m[15]}  // 4th row
	};

	// prepare storage for the inverted mat
	T irows[nrows][nrows] = {
		{mInv.m[0], mInv.m[4], mInv.m[ 8], mInv.m[12]},
		{mInv.m[1], mInv.m[5], mInv.m[ 9], mInv.m[13]},
		{mInv.m[2], mInv.m[6], mInv.m[10], mInv.m[14]},
		{mInv.m[3], mInv.m[7], mInv.m[11], mInv.m[15]}
	};

	// arrays of pointers to each row in <mrows> and <irows>
	T* rmptrs[nrows] = {mrows[0], mrows[1], mrows[2], mrows[3]};
	T* riptrs[nrows] = {irows[0], irows[1], irows[2], irows[3]};
	// temporary row pointer
	T* riptr = 0x0;

	for (int row = 0; row < nrows; row++) {
		int idx = Pivot(row,   mrows, rmptrs);

		if (idx == -1) {
			// error: matrix is singular
			return mInv;
		}

		if (idx != 0) {
			// "swap" rows <row> and <idx> of the to-be
			// inverted mat by exchanging their row
			// pointers (so that riptrs[row] points to
			// a different row after the swap)
			riptr       = riptrs[row];
			riptrs[row] = riptrs[idx];
			riptrs[idx] = riptr;
		}

		T a1 = rmptrs[row][row];

		for (int j = 0; j < nrows; j++) {
			rmptrs[row][j] /= a1;
			riptrs[row][j] /= a1;
		}

		for (int i = 0; i < nrows; i++)
			if (i != row) {
				T a2 = rmptrs[i][row];

				for (int j = 0; j < nrows; j++) {
					rmptrs[i][j] -= (a2 * rmptrs[row][j]);
					riptrs[i][j] -= (a2 * riptrs[row][j]);
				}
			}
	}

	// copy the elements of irows to mInv
	// 1st column
	mInv.m[ 0] = riptrs[0][0];
	mInv.m[ 1] = riptrs[1][0];
	mInv.m[ 2] = riptrs[2][0];
	mInv.m[ 3] = riptrs[3][0];

	// 2nd column
	mInv.m[ 4] = riptrs[0][1];
	mInv.m[ 5] = riptrs[1][1];
	mInv.m[ 6] = riptrs[2][1];
	mInv.m[ 7] = riptrs[3][1];

	// 3rd column
	mInv.m[ 8] = riptrs[0][2];
	mInv.m[ 9] = riptrs[1][2];
	mInv.m[10] = riptrs[2][2];
	mInv.m[11] = riptrs[3][2];

	// 4th column
	mInv.m[12] = riptrs[0][3];
	mInv.m[13] = riptrs[1][3];
	mInv.m[14] = riptrs[2][3];
	mInv.m[15] = riptrs[3][3];

	return mInv;
}
*/



// specialization for floats
template<> const char* mat44<float>::str() const {
	static std::string s;
	std::stringstream ss;
		ss << m[0] << " " << m[4] << " " << m[ 8] << " " << m[12] << std::endl;
		ss << m[1] << " " << m[5] << " " << m[ 9] << " " << m[13] << std::endl;
		ss << m[2] << " " << m[6] << " " << m[10] << " " << m[14] << std::endl;
		ss << m[3] << " " << m[7] << " " << m[11] << " " << m[15] << std::endl;
	s = ss.str();

	return (s.c_str());
}
