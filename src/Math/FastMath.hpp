#ifndef PFFG_FASTMATH_HDR
#define PFFG_FASTMATH_HDR

namespace fastmath {
	// returns (1.0f / sqrt(x))
	inline float isqrt1(float x) {
		float xh = 0.5f * x;
		int i = *(int*) &x;
		i = 0x5f375a86 - (i >> 1);
		x = *(float*) &i;
		x = x * (1.5f - xh * (x * x));
		return x;
	}

	inline float isqrt2(float x) {
		float xh = 0.5f * x;
		int i = *(int*) &x;
		i = 0x5f375a86 - (i >> 1);
		x = *(float*) &i;
		x = x * (1.5f - xh * (x * x));
		x = x * (1.5f - xh * (x * x));
		return x;
	}

	inline float sqrt1(float x) { return (isqrt1(x) * x); }
	inline float sqrt2(float x) { return (isqrt2(x) * x); }
}

#endif
