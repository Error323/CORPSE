#ifndef PFFG_BYTEORDER_HDR
#define PFFG_BYTEORDER_HDR

#include <cstring>

#if defined(__linux__)
#include <byteswap.h>


#if __BYTE_ORDER == __BIG_ENDIAN
#define swabword(w)  (bswap_16(w))
#define swabdword(w) (bswap_32(w))

static inline float swabfloat(float w) {
	char octets[4];
	char ret_octets[4];
	float ret;

	memcpy(octets, &w, 4);

	ret_octets[0] = octets[3];
	ret_octets[1] = octets[2];
	ret_octets[2] = octets[1];
	ret_octets[3] = octets[0];

	memcpy(&ret, ret_octets, 4);
	return ret;
}

#else
#define swabword(w)     (w)
#define swabdword(w)    (w)
#define swabfloat(w)    (w)
#endif


#else
// empty versions for Win32
#define swabword(w)     (w)
#define swabdword(w)    (w)
#define swabfloat(w)    (w)

#endif

#endif
