#ifndef SPR_VERTEXARRAY_HDR
#define SPR_VERTEXARRAY_HDR

#include <GL/glew.h>
#include <GL/gl.h>
#include "../Math/vec3fwd.hpp"
#include "../Math/vec3.hpp"

#define VA_INIT_VERTICES 1000
#define VA_INIT_STRIPS 100
#define VA_SIZE_0 3
#define VA_SIZE_C 4
#define VA_SIZE_T 5
#define VA_SIZE_TN 8
#define VA_SIZE_TC 6

class SprVertexArray {
	public:
		SprVertexArray();
		~SprVertexArray();

		void Initialize();


		inline void AddVertexTC(const vec3f& pos, float tx, float ty, unsigned char* color);
		void DrawArrayTC(int drawType, int stride = 24);

		inline void AddVertexTN(const vec3f& pos, float tx, float ty, const vec3f& norm);
		void DrawArrayTN(int drawType, int stride = 32);

		inline void AddVertexT2(const vec3f& pos, float t1x, float t1y, float t2x, float t2y);
		void DrawArrayT2(int drawType, int stride = 28);

		inline void AddVertexT(const vec3f& pos, float tx, float ty);
		void DrawArrayT(int drawType, int stride = 20);

		inline void AddVertexN(const vec3f& pos, const vec3f& norm);
		void DrawArrayN(int drawType, int stride = 24);

		inline void AddVertex0(const vec3f& pos);
		void DrawArray0(int drawType, int stride = 12);

		inline void AddVertexC(const vec3f& pos, unsigned char* color);
		void DrawArrayC(int drawType, int stride = 16);

		void EndStrip();


		//! these are same as the non-Q* versions of AddVertex*,
		//! but lack automated calling of CheckEnlargeDrawArray
		inline void AddVertexQ0(float x, float y, float z);
		inline void AddVertexQC(const vec3f& pos, unsigned char* color);
		inline void AddVertexQT(const vec3f& pos, float tx, float ty);
		inline void AddVertexQN(const vec3f& pos, const vec3f& norm);
		inline void AddVertexQTN(const vec3f &pos, float tx, float ty, const vec3f& norm);
		inline void AddVertexQTC(const vec3f &pos, float tx, float ty, unsigned char* color);


		//! same as EndStrip(), but without automated EnlargeStripArray
		inline void EndStripQ();


		inline int drawIndex();
		inline void EnlargeArrays(int vertexes, int strips, int stripsize = VA_SIZE_0);

		float* drawArray;
		float* drawArrayPos;
		float* drawArraySize;
		int* stripArray;
		int* stripArrayPos;
		int* stripArraySize;

	protected:
		inline void DrawArrays(int drawType, int stride);
		inline void CheckEnlargeDrawArray();
		void EnlargeStripArray();
		void EnlargeDrawArray();
		void CheckEndStrip();
};

inline void SprVertexArray::DrawArrays(int drawType, int stride) {
	int newIndex, oldIndex = 0;
	int* stripArrayPtr = stripArray;

	while (stripArrayPtr < stripArrayPos) {
		newIndex = (*stripArrayPtr++) / stride;
		glDrawArrays(drawType, oldIndex, newIndex - oldIndex);
		oldIndex = newIndex;
	}
}

inline void SprVertexArray::CheckEnlargeDrawArray() {
	if ((char*) drawArrayPos>(char*) drawArraySize - 10 * sizeof(float))
		EnlargeDrawArray();
}

inline void SprVertexArray::EnlargeArrays(int vertexes, int strips, int stripsize) {
	while ((char*) drawArrayPos>(char*) drawArraySize - stripsize * sizeof(float) * vertexes)
		EnlargeDrawArray();

	while ((char*) stripArrayPos > (char*) stripArraySize - sizeof(int) * strips)
		EnlargeStripArray();
}




inline void SprVertexArray::AddVertexQ0(float x, float y, float z) {
	// assert(drawArraySize >= drawArrayPos + VA_SIZE_0);
	*drawArrayPos++ = x;
	*drawArrayPos++ = y;
	*drawArrayPos++ = z;
}

inline void SprVertexArray::AddVertexQC(const vec3f& pos, unsigned char* color) {
	// assert(drawArraySize >= drawArrayPos + VA_SIZE_C);
	*drawArrayPos++ = pos.x;
	*drawArrayPos++ = pos.y;
	*drawArrayPos++ = pos.z;
	*drawArrayPos++ = *((float*) (color));
}

inline void SprVertexArray::AddVertexQT(const vec3f& pos, float tx, float ty) {
	// assert(drawArraySize >= drawArrayPos + VA_SIZE_T);
	*drawArrayPos++ = pos.x;
	*drawArrayPos++ = pos.y;
	*drawArrayPos++ = pos.z;
	*drawArrayPos++ = tx;
	*drawArrayPos++ = ty;
}

inline void SprVertexArray::AddVertexQN(const vec3f& pos, const vec3f& norm) {
	// assert(drawArraySize >= drawArrayPos + VA_SIZE_TN);
	*drawArrayPos++ = pos.x;
	*drawArrayPos++ = pos.y;
	*drawArrayPos++ = pos.z;
	*drawArrayPos++ = norm.x;
	*drawArrayPos++ = norm.y;
	*drawArrayPos++ = norm.z;
}

inline void SprVertexArray::AddVertexQTN(const vec3f& pos, float tx, float ty, const vec3f& norm) {
	// assert(drawArraySize >= drawArrayPos + VA_SIZE_TN);
	*drawArrayPos++ = pos.x;
	*drawArrayPos++ = pos.y;
	*drawArrayPos++ = pos.z;
	*drawArrayPos++ = tx;
	*drawArrayPos++ = ty;
	*drawArrayPos++ = norm.x;
	*drawArrayPos++ = norm.y;
	*drawArrayPos++ = norm.z;
}

inline void SprVertexArray::AddVertexQTC(const vec3f& pos, float tx, float ty, unsigned char* col) {
	// assert(drawArraySize >= drawArrayPos + VA_SIZE_TC);
	*drawArrayPos++ = pos.x;
	*drawArrayPos++ = pos.y;
	*drawArrayPos++ = pos.z;
	*drawArrayPos++ = tx;
	*drawArrayPos++ = ty;
	*drawArrayPos++ = *((float*) (col));
}

inline void SprVertexArray::AddVertex0(const vec3f& pos) {
	if (drawArrayPos > drawArraySize - 10)
		EnlargeDrawArray();

	*drawArrayPos++ = pos.x;
	*drawArrayPos++ = pos.y;
	*drawArrayPos++ = pos.z;
}

inline void SprVertexArray::AddVertexC(const vec3f& pos, unsigned char* color) {
	CheckEnlargeDrawArray();
	*drawArrayPos++ = pos.x;
	*drawArrayPos++ = pos.y;
	*drawArrayPos++ = pos.z;
	*drawArrayPos++ = *((float*) (color));
}

inline void SprVertexArray::AddVertexT(const vec3f& pos, float tx, float ty) {
	CheckEnlargeDrawArray();
	*drawArrayPos++ = pos.x;
	*drawArrayPos++ = pos.y;
	*drawArrayPos++ = pos.z;
	*drawArrayPos++ = tx;
	*drawArrayPos++ = ty;
}

inline void SprVertexArray::AddVertexN(const vec3f& pos, const vec3f& norm) {
	CheckEnlargeDrawArray();
	*drawArrayPos++ = pos.x;
	*drawArrayPos++ = pos.y;
	*drawArrayPos++ = pos.z;
	*drawArrayPos++ = norm.x;
	*drawArrayPos++ = norm.y;
	*drawArrayPos++ = norm.z;
}

inline void SprVertexArray::AddVertexT2(const vec3f& pos, float tx, float ty, float t2x, float t2y) {
	CheckEnlargeDrawArray();
	*drawArrayPos++ = pos.x;
	*drawArrayPos++ = pos.y;
	*drawArrayPos++ = pos.z;
	*drawArrayPos++ = tx;
	*drawArrayPos++ = ty;
	*drawArrayPos++ = t2x;
	*drawArrayPos++ = t2y;
}

inline void SprVertexArray::AddVertexTN(const vec3f& pos, float tx, float ty, const vec3f& norm) {
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

inline void SprVertexArray::AddVertexTC(const vec3f& pos, float tx, float ty, unsigned char* col) {
	CheckEnlargeDrawArray();
	*drawArrayPos++ = pos.x;
	*drawArrayPos++ = pos.y;
	*drawArrayPos++ = pos.z;
	*drawArrayPos++ = tx;
	*drawArrayPos++ = ty;
	*drawArrayPos++ = *((float*) (col));
}




inline void SprVertexArray::CheckEndStrip() {
	if (stripArrayPos == stripArray || *(stripArrayPos - 1) != ((char*) drawArrayPos - (char*) drawArray))
		EndStrip();
}

inline int SprVertexArray::drawIndex() {
	return (drawArrayPos - drawArray);
}

inline void SprVertexArray::EndStripQ() {
	// assert(stripArraySize >= stripArrayPos + 1);
	*stripArrayPos++ = ((char*) drawArrayPos - (char*) drawArray);
}

#endif
