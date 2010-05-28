#ifndef PFFG_VEC3FWD_HDR
#define PFFG_VEC3FWD_HDR

// typedefs introduce a new name for existing type
// while forward declarations introduce a new type
// so...
template<typename T> struct vec2;
template<typename T> struct vec3;
typedef vec3<int> vec3i;
typedef vec3<float> vec3f;

#endif
