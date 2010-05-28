// memcpy() is defined in the string header
#include <cstring>
#include "./SprVertexArray.hpp"

SprVertexArray::SprVertexArray() {
	drawArray = new float[VA_INIT_VERTICES];
	stripArray = new int[VA_INIT_STRIPS];
	Initialize();
	drawArraySize = drawArray + VA_INIT_VERTICES;
	stripArraySize = stripArray + VA_INIT_STRIPS;
}

SprVertexArray::~SprVertexArray() {
	delete[] drawArray;
	delete[] stripArray;
}

void SprVertexArray::Initialize() {
	drawArrayPos = drawArray;
	stripArrayPos = stripArray;
}

void SprVertexArray::EndStrip() {
	if  ((char*) stripArrayPos>(char*) stripArraySize - 4 * sizeof(int))
		EnlargeStripArray();

	*stripArrayPos++ = ((char*) drawArrayPos - (char*) drawArray);
}



// void DrawArray0(int drawType, int stride = 12);
void SprVertexArray::DrawArray0(int drawType, int stride) {
	CheckEndStrip();
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, stride, drawArray);
	DrawArrays(drawType, stride);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void SprVertexArray::DrawArrayC(int drawType, int stride) {
	CheckEndStrip();
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(3, GL_FLOAT, stride, drawArray);
	glColorPointer(4, GL_UNSIGNED_BYTE, stride, drawArray + 3);
	DrawArrays(drawType, stride);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

void SprVertexArray::DrawArrayT(int drawType, int stride) {
	CheckEndStrip();
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, stride, drawArray);
	glTexCoordPointer(2, GL_FLOAT, stride, drawArray + 3);
	DrawArrays(drawType, stride);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void SprVertexArray::DrawArrayT2(int drawType, int stride) {
	CheckEndStrip();
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, stride, drawArray);
	glTexCoordPointer(2, GL_FLOAT, stride, drawArray + 3);

	glClientActiveTextureARB(GL_TEXTURE1_ARB);
	glTexCoordPointer(2, GL_FLOAT, stride, drawArray + 5);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glClientActiveTextureARB(GL_TEXTURE0_ARB);
	DrawArrays(drawType, stride);
	glClientActiveTextureARB(GL_TEXTURE1_ARB);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glClientActiveTextureARB(GL_TEXTURE0_ARB);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void SprVertexArray::DrawArrayTN(int drawType, int stride) {
	CheckEndStrip();
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glVertexPointer(3, GL_FLOAT, stride, drawArray);
	glTexCoordPointer(2, GL_FLOAT, stride, drawArray + 3);
	glNormalPointer(GL_FLOAT, stride, drawArray + 5);
	DrawArrays(drawType, stride);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
}

void SprVertexArray::DrawArrayTC(int drawType, int stride) {
	CheckEndStrip();
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glVertexPointer(3, GL_FLOAT, stride, drawArray);
	glTexCoordPointer(2, GL_FLOAT, stride, drawArray + 3);
	glColorPointer(4, GL_UNSIGNED_BYTE, stride, drawArray + 5);
	DrawArrays(drawType, stride);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

void SprVertexArray::DrawArrayN(int drawType, int stride) {
	CheckEndStrip();
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glVertexPointer(3, GL_FLOAT, stride, drawArray);
	glNormalPointer(GL_FLOAT, stride, drawArray + 3);
	DrawArrays(drawType, stride);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
}



/*
void SprVertexArrayRange::DrawArrayTN(int drawType, int stride) {
	CheckEndStrip();
	// glEnableClientState(GL_VERTEX_ARRAY_RANGE_NV);
	glVertexPointer(3, GL_FLOAT, stride, &drawArray[0]);
	glTexCoordPointer(2, GL_FLOAT, stride, &drawArray[3]);
	glNormalPointer(GL_FLOAT, stride, &drawArray[5]);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	DrawArrays(drawType, stride);
	glSetFenceNV(fence, GL_ALL_COMPLETED_NV);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	// glDisableClientState(GL_VERTEX_ARRAY_RANGE_NV);
}
*/



void SprVertexArray::EnlargeDrawArray() {
	int pos = drawArrayPos - drawArray;
	int oldsize = drawArraySize - drawArray;
	int newsize = oldsize * 2;
	float* tempArray = new float[newsize];
	memcpy(tempArray, drawArray, oldsize * sizeof(float));

	delete[] drawArray;
	drawArray = tempArray;
	drawArraySize = drawArray + newsize;
	drawArrayPos = drawArray + pos;
}

void SprVertexArray::EnlargeStripArray() {
	int pos = stripArrayPos - stripArray;
	int oldsize = stripArraySize - stripArray;
	int newsize = oldsize * 2;

	int* tempArray = new int[newsize];
	memcpy(tempArray, stripArray, oldsize * sizeof(int));

	delete[] stripArray;
	stripArray = tempArray;
	stripArraySize = stripArray + newsize;
	stripArrayPos = stripArray + pos;
}
