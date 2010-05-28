#ifndef PFFG_VEC4FWD_HDR
#define PFFG_VEC4FWD_HDR

// typedefs introduce a new name for existing type
// while forward declarations introduce a new type
// so...
template<typename T> struct vec4;
typedef vec4<int> vec4i;
typedef vec4<float> vec4f;

#endif
