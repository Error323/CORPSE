#ifndef PFFG_VERTEXARRAY_HDR
#define PFFG_VERTEXARRAY_HDR

// #include <GL/glew.h>
#include <GL/gl.h>
#include "../Math/vec3fwd.hpp"
#include "../Math/vec3.hpp"
#include "../System/Debugger.hpp"

#define VA_INIT_VERTICES 1000
#define VA_INIT_STRIPS 100

// number of elements (bytes / 4) per vertex
#define VA_SIZE_2D0  2
#define VA_SIZE_0    3
#define VA_SIZE_C    4
#define VA_SIZE_T    5
#define VA_SIZE_N    6
#define VA_SIZE_TN   8
#define VA_SIZE_TNT 14
#define VA_SIZE_TC   6
#define VA_SIZE_2DT  4

class VertexArray {
public:
	typedef void (*StripCallback)(void* data);

public:
	VertexArray(unsigned int maxVerts = 1 << 16);
	~VertexArray();
	void Initialize();

	inline void AddVertex0(const vec3f& p);
	inline void AddVertex0(float x, float y, float z);
	inline void AddVertex2d0(float x, float z);
	inline void AddVertexN(const vec3f& p, const vec3f& normal);
	inline void AddVertexT(const vec3f& p, float tx, float ty);
	inline void AddVertexC(const vec3f& p, const unsigned char* color);
	inline void AddVertexTC(const vec3f& p, float tx, float ty, const unsigned char* color);
	inline void AddVertexTN(const vec3f& p, float tx, float ty, const vec3f& n);
	inline void AddVertexTNT(const vec3f& p, float tx, float ty, const vec3f& n, const vec3f& st, const vec3f& tt); 
	inline void AddVertexT2(const vec3f& p, float t1x, float t1y, float t2x, float t2y);
	inline void AddVertex2dT(float x, float y, float tx, float ty);

	void DrawArray0(const int drawType, unsigned int stride = 12);
	void DrawArray2d0(const int drawType, unsigned int stride = 8);
	void DrawArrayN(const int drawType, unsigned int stride = 24);
	void DrawArrayT(const int drawType, unsigned int stride = 20);
	void DrawArrayC(const int drawType, unsigned int stride = 16);
	void DrawArrayTC(const int drawType, unsigned int stride = 24);
	void DrawArrayTN(const int drawType, unsigned int stride = 32);
	void DrawArrayTNT(const int drawType, unsigned int stride = 56);
	void DrawArrayT2(const int drawType, unsigned int stride = 28);
	void DrawArray2dT(const int drawType, unsigned int stride = 16);
	void DrawArray2dT(const int drawType, StripCallback callback, void* data, unsigned int stride = 16);

	//! same as the AddVertex... functions but without automated CheckEnlargeDrawArray
	inline void AddVertexQ0(float x, float y, float z);
	inline void AddVertexQ0(const vec3f& f3) { AddVertexQ0(f3.x, f3.y, f3.z); }
	inline void AddVertex2dQ0(float x, float z);
	inline void AddVertexQN(const vec3f& p, const vec3f& n);
	inline void AddVertexQC(const vec3f& p, const unsigned char* color);
	inline void AddVertexQT(const vec3f& p, float tx, float ty);
	inline void AddVertex2dQT(float x, float y, float tx, float ty);
	inline void AddVertexQTN(const vec3f& p, float tx, float ty, const vec3f& n);
	inline void AddVertexQTNT(const vec3f& p, float tx, float ty, const vec3f& n, const vec3f& st, const vec3f& tt);
	inline void AddVertexQTC(const vec3f& p, float tx, float ty, const unsigned char* color);

	//! same as EndStrip, but without automated EnlargeStripArray
	inline void EndStripQ();

	bool IsReady();
	inline unsigned int drawIndex() const;
	void EndStrip();
	inline void EnlargeArrays(const unsigned int vertices, const unsigned int strips, const unsigned int stripsize = VA_SIZE_0);

	float* drawArray;
	float* drawArrayPos;
	float* drawArraySize;

	unsigned int* stripArray;
	unsigned int* stripArrayPos;
	unsigned int* stripArraySize;

	unsigned int maxVertices;

protected:
	void DrawArrays(const GLenum mode, const unsigned int stride);
	void DrawArraysCallback(const GLenum mode, const unsigned int stride, StripCallback callback, void* data);
	inline void CheckEnlargeDrawArray();
	void EnlargeStripArray();
	void EnlargeDrawArray();
	inline void CheckEndStrip();
};



#ifdef DEBUG
	#define ASSERT_SIZE(x) PFFG_ASSERT(drawArraySize >= (drawArrayPos + x));
#else
	#define ASSERT_SIZE(x)
#endif



void VertexArray::CheckEnlargeDrawArray() {
	if ((char*) drawArrayPos > ((char*) drawArraySize - 10 * sizeof(float)))
		EnlargeDrawArray();
}

void VertexArray::EnlargeArrays(const unsigned int vertices, const unsigned int strips, const unsigned int stripsize) {
	while ((char*) drawArrayPos > ((char*) drawArraySize - stripsize * sizeof(float) * vertices))
		EnlargeDrawArray();

	while ((char*) stripArrayPos > ((char*) stripArraySize - sizeof(unsigned int) * strips))
		EnlargeStripArray();
}



void VertexArray::AddVertexQ0(float x, float y, float z) {
	ASSERT_SIZE(VA_SIZE_0)
	*drawArrayPos++ = x;
	*drawArrayPos++ = y;
	*drawArrayPos++ = z;
}

void VertexArray::AddVertex2dQ0(float x, float z) {
	ASSERT_SIZE(VA_SIZE_2D0)
	*drawArrayPos++ = x;
	*drawArrayPos++ = z;
}

void VertexArray::AddVertexQN(const vec3f& pos, const vec3f& normal) {
	ASSERT_SIZE(VA_SIZE_0)
	*drawArrayPos++ = pos.x;
	*drawArrayPos++ = pos.y;
	*drawArrayPos++ = pos.z;
	*drawArrayPos++ = normal.x;
	*drawArrayPos++ = normal.y;
	*drawArrayPos++ = normal.z;
}

void VertexArray::AddVertexQC(const vec3f& pos, const unsigned char* color) {
	ASSERT_SIZE(VA_SIZE_C)
	*drawArrayPos++ = pos.x;
	*drawArrayPos++ = pos.y;
	*drawArrayPos++ = pos.z;
	*drawArrayPos++ = *((float*) (color));
}

void VertexArray::AddVertexQT(const vec3f& pos, float tx, float ty) {
	ASSERT_SIZE(VA_SIZE_T)
	*drawArrayPos++ = pos.x;
	*drawArrayPos++ = pos.y;
	*drawArrayPos++ = pos.z;
	*drawArrayPos++ = tx;
	*drawArrayPos++ = ty;
}

void VertexArray::AddVertexQTN(const vec3f& pos, float tx, float ty, const vec3f& norm) {
	ASSERT_SIZE(VA_SIZE_TN)
	*drawArrayPos++ = pos.x;
	*drawArrayPos++ = pos.y;
	*drawArrayPos++ = pos.z;
	*drawArrayPos++ = tx;
	*drawArrayPos++ = ty;
	*drawArrayPos++ = norm.x;
	*drawArrayPos++ = norm.y;
	*drawArrayPos++ = norm.z;
}

void VertexArray::AddVertexQTNT(const vec3f& p, float tx, float ty, const vec3f& n, const vec3f& st, const vec3f& tt) {
	ASSERT_SIZE(VA_SIZE_TNT)
	*drawArrayPos++ = p.x;
	*drawArrayPos++ = p.y;
	*drawArrayPos++ = p.z;
	*drawArrayPos++ = tx;
	*drawArrayPos++ = ty;
	*drawArrayPos++ = n.x;
	*drawArrayPos++ = n.y;
	*drawArrayPos++ = n.z;
	*drawArrayPos++ = st.x;
	*drawArrayPos++ = st.y;
	*drawArrayPos++ = st.z;
	*drawArrayPos++ = tt.x;
	*drawArrayPos++ = tt.y;
	*drawArrayPos++ = tt.z;
}

void VertexArray::AddVertexQTC(const vec3f& pos, float tx, float ty, const unsigned char* col) {
	ASSERT_SIZE(VA_SIZE_TC)
	*drawArrayPos++ = pos.x;
	*drawArrayPos++ = pos.y;
	*drawArrayPos++ = pos.z;
	*drawArrayPos++ = tx;
	*drawArrayPos++ = ty;
	*drawArrayPos++ = *((float*)(col));
}

void VertexArray::AddVertex2dQT(float x, float y, float tx, float ty) {
	ASSERT_SIZE(VA_SIZE_2DT)
	*drawArrayPos++ = x;
	*drawArrayPos++ = y;
	*drawArrayPos++ = tx;
	*drawArrayPos++ = ty;
}

void VertexArray::AddVertex0(const vec3f& pos) {
	CheckEnlargeDrawArray();
	*drawArrayPos++ = pos.x;
	*drawArrayPos++ = pos.y;
	*drawArrayPos++ = pos.z;
}

void VertexArray::AddVertex0(float x, float y, float z) {
	CheckEnlargeDrawArray();
	*drawArrayPos++ = x;
	*drawArrayPos++ = y;
	*drawArrayPos++ = z;
}

void VertexArray::AddVertex2d0(float x, float z) {
	CheckEnlargeDrawArray();
	*drawArrayPos++ = x;
	*drawArrayPos++ = z;
}

void VertexArray::AddVertexN(const vec3f& pos, const vec3f& normal) {
	CheckEnlargeDrawArray();
	*drawArrayPos++ = pos.x;
	*drawArrayPos++ = pos.y;
	*drawArrayPos++ = pos.z;
	*drawArrayPos++ = normal.x;
	*drawArrayPos++ = normal.y;
	*drawArrayPos++ = normal.z;
}

void VertexArray::AddVertexC(const vec3f& pos, const unsigned char* color) {
	CheckEnlargeDrawArray();
	*drawArrayPos++ = pos.x;
	*drawArrayPos++ = pos.y;
	*drawArrayPos++ = pos.z;
	*drawArrayPos++ = *((float*)(color));
}

void VertexArray::AddVertexT(const vec3f& pos, float tx, float ty) {
	CheckEnlargeDrawArray();
	*drawArrayPos++ = pos.x;
	*drawArrayPos++ = pos.y;
	*drawArrayPos++ = pos.z;
	*drawArrayPos++ = tx;
	*drawArrayPos++ = ty;
}

void VertexArray::AddVertexT2(const vec3f& pos, float tx, float ty, float t2x, float t2y) {
	CheckEnlargeDrawArray();
	*drawArrayPos++ = pos.x;
	*drawArrayPos++ = pos.y;
	*drawArrayPos++ = pos.z;
	*drawArrayPos++ = tx;
	*drawArrayPos++ = ty;
	*drawArrayPos++ = t2x;
	*drawArrayPos++ = t2y;
}

void VertexArray::AddVertexTN(const vec3f& pos, float tx, float ty, const vec3f& norm) {
	CheckEnlargeDrawArray();
	*drawArrayPos++ = pos.x;
	*drawArrayPos++ = pos.y;
	*drawArrayPos++ = pos.z;
	*drawArrayPos++ = tx;
	*drawArrayPos++ = ty;
	*drawArrayPos++ = norm.x;
	*drawArrayPos++ = norm.y;
	*drawArrayPos++ = norm.z;
}

void VertexArray::AddVertexTNT(const vec3f& p, float tx, float ty, const vec3f& n, const vec3f& st, const vec3f& tt) {
	CheckEnlargeDrawArray();
	*drawArrayPos++ = p.x;
	*drawArrayPos++ = p.y;
	*drawArrayPos++ = p.z;
	*drawArrayPos++ = tx;
	*drawArrayPos++ = ty;
	*drawArrayPos++ = n.x;
	*drawArrayPos++ = n.y;
	*drawArrayPos++ = n.z;
	*drawArrayPos++ = st.x;
	*drawArrayPos++ = st.y;
	*drawArrayPos++ = st.z;
	*drawArrayPos++ = tt.x;
	*drawArrayPos++ = tt.y;
	*drawArrayPos++ = tt.z;
}

void VertexArray::AddVertexTC(const vec3f& pos, float tx, float ty, const unsigned char* col) {
	CheckEnlargeDrawArray();
	*drawArrayPos++ = pos.x;
	*drawArrayPos++ = pos.y;
	*drawArrayPos++ = pos.z;
	*drawArrayPos++ = tx;
	*drawArrayPos++ = ty;
	*drawArrayPos++ = *((float*)(col));
}

void VertexArray::AddVertex2dT(float x, float y, float tx, float ty) {
	CheckEnlargeDrawArray();
	*drawArrayPos++ = x;
	*drawArrayPos++ = y;
	*drawArrayPos++ = tx;
	*drawArrayPos++ = ty;
}


void VertexArray::CheckEndStrip() {
	if (stripArrayPos == stripArray || ((ptrdiff_t) * (stripArrayPos - 1)) != ((char*) drawArrayPos - (char*) drawArray))
		EndStrip();
}

unsigned int VertexArray::drawIndex() const {
	return drawArrayPos-drawArray;
}

void VertexArray::EndStripQ() {
	PFFG_ASSERT(stripArraySize >= stripArrayPos + 1);

	*stripArrayPos++ = ((char*) drawArrayPos - (char*) drawArray);
}

#endif
