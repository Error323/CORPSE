#include <ostream>
#include <fstream>
#include <string>
#include <cstring>

#include <IL/il.h>
#include <IL/ilu.h>

#include <GL/glew.h>
#include <GL/gl.h>

#include "./BitMap.hpp"
#include "./NVDDS.hpp"
#include "../GL.hpp"
#include "../../Math/vec3fwd.hpp"
#include "../../Math/vec3.hpp"
#include "../../System/BitOps.hpp"
#include "../../System/FileHandler.hpp"



struct InitializeOpenIL {
	InitializeOpenIL() {
		ilInit();
		iluInit();
	}

	~InitializeOpenIL() {
		ilShutDown();
	}
} static initOpenIL;



CBitMap::CBitMap(): xsize(0), ysize(0) {
	mem = 0;
	ddsimage = 0;
	type = BitmapTypeStandardRGBA;
}

CBitMap::~CBitMap() {
	delete[] mem;
	delete ddsimage;
}

CBitMap::CBitMap(const CBitMap& old) {
	assert(old.type != BitmapTypeDDS);
	ddsimage = 0;
	type = old.type;
	xsize = old.xsize;
	ysize = old.ysize;
	int size;

	if (type == BitmapTypeStandardRGBA) {
		size = xsize * ysize * 4;
	} else {
		// alpha?
		size = xsize * ysize;
	}

	mem=new unsigned char[size];
	memcpy(mem,old.mem,size);
}


CBitMap::CBitMap(unsigned char *data, int xsize, int ysize): xsize(xsize), ysize(ysize) {
	type = BitmapTypeStandardRGBA;
	ddsimage = 0;
	mem = new unsigned char[xsize * ysize * 4];
	memcpy(mem, data, xsize * ysize * 4);
}


CBitMap& CBitMap::operator=(const CBitMap& bm) {
	if (this != &bm) {
		delete[] mem;
		xsize = bm.xsize;
		ysize = bm.ysize;
		int size;

		if (type == BitmapTypeStandardRGBA) {
			size = xsize * ysize * 4;
		} else {
			// alpha?
			size = xsize * ysize;
		}

		mem = new unsigned char[size];
		memcpy(mem, bm.mem, size);
	}

	return *this;
}



void CBitMap::Alloc(int w, int h) {
	delete[] mem;
	xsize = w;
	ysize = h;
	type = BitmapTypeStandardRGBA;
	mem = new unsigned char[w * h * 4];
	memset(mem, 0, w * h * 4);
}


bool CBitMap::Load(const std::string& filename, unsigned char defaultAlpha) {
	bool noAlpha = true;

	delete[] mem;
	mem = NULL;

	if (filename.find(".dds") != std::string::npos) {
		ddsimage = new nv_dds::CDDSImage();
		type = BitmapTypeDDS;
		return ddsimage->load(filename);
	}

	type = BitmapTypeStandardRGBA;

	CFileHandler file(filename);
	if (!file.FileExists()) {
		Alloc(1, 1);
		return false;
	}

	unsigned char* buffer = new unsigned char[file.FileSize() + 2];
	file.Read(buffer, file.FileSize());

	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
	ilEnable(IL_ORIGIN_SET);

	ILuint ImageName = 0;
	ilGenImages(1, &ImageName);
	ilBindImage(ImageName);

	const bool success = !!ilLoadL(IL_TYPE_UNKNOWN, buffer, file.FileSize());
	delete [] buffer;

	if (!success) {
		xsize = 1;
		ysize = 1;
		mem = new unsigned char[4];
		// allow us to easily see textures that failed to load
		mem[0] = 255;
		mem[1] = 0;
		mem[2] = 0;
		mem[3] = 255;
		return false;
	}

	noAlpha = (ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL) != 4);
	ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
	xsize = ilGetInteger(IL_IMAGE_WIDTH);
	ysize = ilGetInteger(IL_IMAGE_HEIGHT);

	mem = new unsigned char[xsize * ysize * 4];
	// ilCopyPixels(0, 0, 0, xsize, ysize, 0, IL_RGBA, IL_UNSIGNED_BYTE, mem);
	memcpy(mem, ilGetData(), xsize * ysize * 4);

	ilDeleteImages(1, &ImageName);

	if (noAlpha){
		for (int y = 0; y < ysize; ++y) {
			for (int x = 0; x < xsize; ++x) {
				mem[((y * xsize + x) * 4) + 3] = defaultAlpha;
			}
		}
	}

	return true;
}


bool CBitMap::LoadGrayscale(const std::string& filename) {
	type = BitmapTypeStandardAlpha;

	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
	ilEnable(IL_ORIGIN_SET);

	CFileHandler file(filename);
	if (!file.FileExists()) {
		return false;
	}

	unsigned char* buffer = new unsigned char[file.FileSize() + 1];
	file.Read(buffer, file.FileSize());

	ILuint ImageName = 0;
	ilGenImages(1, &ImageName);
	ilBindImage(ImageName);

	const bool success = !!ilLoadL(IL_TYPE_UNKNOWN, buffer, file.FileSize());
	delete [] buffer;

	if (!success)
		return false;

	ilConvertImage(IL_LUMINANCE, IL_UNSIGNED_BYTE);
	xsize = ilGetInteger(IL_IMAGE_WIDTH);
	ysize = ilGetInteger(IL_IMAGE_HEIGHT);

	mem = new unsigned char[xsize * ysize];
	memcpy(mem, ilGetData(), xsize * ysize);
	
	ilDeleteImages(1, &ImageName);
	
	return true;
}

/*
bool CBitMap::Save(const std::string& filename, bool opaque) {
	if (type == BitmapTypeDDS) {
		return ddsimage->save(filename);
	}

	// Use DevIL on Windows/Linux/...
	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
	ilEnable(IL_ORIGIN_SET);

	unsigned char* buf = new unsigned char[xsize * ysize * 4];
	const int ymax = (ysize - 1);

	// HACK flip the image so it saves the right way up
	// (fiddling with ilOriginFunc didn't do anything),
	// duplicated with ReverseYAxis()
	for (int y = 0; y < ysize; ++y) {
		for (int x = 0; x < xsize; ++x) {
			const int bi = 4 * (x + (xsize * (ymax - y)));
			const int mi = 4 * (x + (xsize * (y)));
			buf[bi + 0] = mem[mi + 0];
			buf[bi + 1] = mem[mi + 1];
			buf[bi + 2] = mem[mi + 2];
			buf[bi + 3] = opaque ? 0xff : mem[mi + 3];
		}
	}

	ilHint(IL_COMPRESSION_HINT, IL_USE_COMPRESSION);
	ilSetInteger(IL_JPG_QUALITY, 100);

	ILuint ImageName = 0;
	ilGenImages(1, &ImageName);
	ilBindImage(ImageName);

	ilTexImage(xsize, ysize, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, NULL);
	ilSetData(buf);

	const std::string fullpath = filesystem.LocateFile(filename, FileSystem::WRITE);
	const bool success = ilSaveImage((char*) fullpath.c_str());

	ilDeleteImages(1, &ImageName);
	delete[] buf;

	return success;
}
*/


#ifndef BITMAP_NO_OPENGL

unsigned int CBitMap::CreateTexture(bool mipmaps) {
	if (type == BitmapTypeDDS) {
		return CreateDDSTexture();
	}

	if (mem == NULL) {
		return 0;
	}

	// WARNING: some drivers return "2.0" as a version string but
	// switch to software rendering for non-power-of-two textures,
	// GL_ARB_texture_non_power_of_two indicates that the hardware
	// will actually support NPOT textures
	if ((unsigned(xsize) != next_power_of_two(xsize) || unsigned(ysize) != next_power_of_two(ysize)) && !GLEW_ARB_texture_non_power_of_two) {
		CBitMap bm = CreateRescaled(next_power_of_two(xsize), next_power_of_two(ysize));
		return bm.CreateTexture(mipmaps);
	}

	GLuint texture;

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (mipmaps) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

		mglBuildMipMaps(GL_TEXTURE_2D, GL_RGBA8, xsize, ysize, GL_RGBA, GL_UNSIGNED_BYTE, mem);
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		// glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, xsize, ysize, 0, GL_RGBA, GL_UNSIGNED_BYTE, mem);
		// gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA8, xsize, ysize, GL_RGBA, GL_UNSIGNED_BYTE, mem);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, xsize, ysize, 0, GL_RGBA, GL_UNSIGNED_BYTE, mem);
	}

	return texture;
}


unsigned int CBitMap::CreateDDSTexture() {
	GLuint texobj;
	glPushAttrib(GL_TEXTURE_BIT);

	glGenTextures(1, &texobj);

	switch (ddsimage->get_type()) {
		case nv_dds::TextureNone: {
			glDeleteTextures(1, &texobj);
			texobj = 0;
		} break;
		case nv_dds::TextureFlat: {
			// 1D, 2D, and rectangle textures
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, texobj);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

			if (!ddsimage->upload_texture2D()) {
				glDeleteTextures(1, &texobj);
				texobj = 0;
			}
		} break;
		case nv_dds::Texture3D: {
			glEnable(GL_TEXTURE_3D);
			glBindTexture(GL_TEXTURE_3D, texobj);

			if (!ddsimage->upload_texture3D()) {
				glDeleteTextures(1, &texobj);
				texobj = 0;
			}
		} break;
		case nv_dds::TextureCubemap: {
			glEnable(GL_TEXTURE_CUBE_MAP_ARB);
			glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, texobj);

			if (!ddsimage->upload_textureCubemap()) {
				glDeleteTextures(1, &texobj);
				texobj = 0;
			}
		} break;
		default: {
			assert(false);
		} break;
	}

	glPopAttrib();
	return texobj;
}

#endif // !BITMAP_NO_OPENGL



void CBitMap::CreateAlpha(unsigned char red, unsigned char green, unsigned char blue) {
	vec3f aCol;

	for (int a = 0; a < 3; ++a) {
		int cCol = 0;
		int numCounted = 0;
		for (int y = 0; y < ysize; ++y) {
			for (int x = 0; x < xsize; ++x) {
				const bool rr = (mem[(y * xsize + x) * 4 + 0] == red);
				const bool gg = (mem[(y * xsize + x) * 4 + 1] == green);
				const bool bb = (mem[(y * xsize + x) * 4 + 2] == blue);

				if (mem[(y * xsize + x) * 4 + 3] != 0 && !(rr && gg && bb)) {
					cCol += mem[(y * xsize + x) * 4 + a];
					++numCounted;
				}
			}
		}
		if (numCounted != 0) {
			*(&aCol.x + a) = cCol / 255.0f / numCounted;
		}
	}
	for (int y = 0; y < ysize; ++y) {
		for (int x = 0; x < xsize; ++x) {
			const bool rr = (mem[(y * xsize + x) * 4 + 0] == red);
			const bool gg = (mem[(y * xsize + x) * 4 + 1] == green);
			const bool bb = (mem[(y * xsize + x) * 4 + 2] == blue);

			if (rr && gg && bb) {
				mem[(y * xsize + x) * 4 + 0] = (unsigned char) (aCol.x * 255);
				mem[(y * xsize + x) * 4 + 1] = (unsigned char) (aCol.y * 255);
				mem[(y * xsize + x) * 4 + 2] = (unsigned char) (aCol.z * 255);
				mem[(y * xsize + x) * 4 + 3] = 0;
			}
		}
	}
}


// only used by GUI
void CBitMap::SetTransparent(unsigned char red, unsigned char green, unsigned char blue) {
	for (int y = 0; y < xsize; y++) {
		for (int x = 0; x < xsize; x++) {
			unsigned int index = (y * xsize + x) * 4;

			if (mem[index + 0] == red &&
				mem[index + 1] == green &&
				mem[index + 2] == blue) {

				mem[index + 3] = 0;
			}
		}
	}
}



/*
// only used by tree generator
void CBitMap::Renormalize(vec3f newCol) {
	vec3f aCol;
	vec3f colorDif;

	for (int a = 0; a < 3; ++a) {
		int cCol=0;
		int numCounted = 0;

		for (int y = 0; y < ysize; ++y) {
			for (int x = 0; x < xsize; ++x) {
				if (mem[(y * xsize + x) * 4 + 3] != 0) {
					cCol += mem[(y * xsize + x) * 4 + a];
					++numCounted;
				}
			}
		}

		*(&aCol.x + a) = cCol / 255.0f / numCounted;
		*(&colorDif.x + a) = (&newCol.x + a) - *(&aCol.x + a);
		cCol /= xsize * ysize;
	}

	for (int a = 0; a < 3; ++a) {
		for (int y = 0; y < ysize; ++y) {
			for (int x = 0; x < xsize; ++x) {
				float nc = float(mem[(y * xsize + x) * 4 + a]) / 255.0f + colorDif[a];
				mem[(y * xsize + x) * 4 + a] = (unsigned char) (std::min(255.0f, std::max(0.0f, nc * 255)));
			}
		}
	}
}
*/

// Unused
CBitMap CBitMap::GetRegion(int startx, int starty, int width, int height) {
	CBitMap bm;

	delete[] bm.mem;
	bm.mem = new unsigned char[width * height * 4];
	bm.xsize = width;
	bm.ysize = height;

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
 			bm.mem[(y * width + x) * 4    ] = mem[((starty + y) * xsize + startx + x) * 4    ];
			bm.mem[(y * width + x) * 4 + 1] = mem[((starty + y) * xsize + startx + x) * 4 + 1];
			bm.mem[(y * width + x) * 4 + 2] = mem[((starty + y) * xsize + startx + x) * 4 + 2];
			bm.mem[(y * width + x) * 4 + 3] = mem[((starty + y) * xsize + startx + x) * 4 + 3];
		}
	}

	return bm;
}



CBitMap CBitMap::CreateMipMapLevel(void) {
	CBitMap bm;

	delete[] bm.mem;
	bm.xsize = xsize / 2;
	bm.ysize = ysize / 2;
	bm.mem = new unsigned char[bm.xsize * bm.ysize * 4];

	for (int y = 0; y < ysize / 2; ++y) {
		for (int x = 0; x < xsize / 2; ++x) {
			float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;

			for (int y2 = 0; y2 < 2; ++y2) {
				for (int x2 = 0; x2 < 2; ++x2) {
					r += mem[((y * 2 + y2) * xsize + x * 2 + x2) * 4 + 0];
					g += mem[((y * 2 + y2) * xsize + x * 2 + x2) * 4 + 1];
					b += mem[((y * 2 + y2) * xsize + x * 2 + x2) * 4 + 2];
					a += mem[((y * 2 + y2) * xsize + x * 2 + x2) * 4 + 3];
				}
			}
			bm.mem[(y * bm.xsize + x) * 4    ] = (unsigned char) (r / 4);
			bm.mem[(y * bm.xsize + x) * 4 + 1] = (unsigned char) (g / 4);
			bm.mem[(y * bm.xsize + x) * 4 + 2] = (unsigned char) (b / 4);
			bm.mem[(y * bm.xsize + x) * 4 + 3] = (unsigned char) (a / 4);
		}
	}

	return bm;

}


CBitMap CBitMap::CreateRescaled(int newx, int newy) {
	CBitMap bm;

	delete[] bm.mem;
	bm.xsize = newx;
	bm.ysize = newy;
	bm.mem = new unsigned char[bm.xsize * bm.ysize * 4];

	float dx = float(xsize) / newx;
	float dy = float(ysize) / newy;
	float cy = 0.0f;

	for (int y = 0; y < newy; ++y) {
		int sy = int(cy); cy += dy;
		int ey = int(cy);

		if (ey == sy)
			ey = sy + 1;

		float cx = 0.0f;

		for (int x = 0; x < newx; ++x) {
			int sx = int(cx); cx += dx;
			int ex = int(cx);

			if (ex == sx)
				ex = sx + 1;

			int r = 0, g = 0, b = 0, a = 0;

			for (int y2 = sy; y2 < ey; ++y2) {
				for (int x2 = sx; x2 < ex; ++x2) {
					r += mem[(y2 * xsize + x2) * 4 + 0];
					g += mem[(y2 * xsize + x2) * 4 + 1];
					b += mem[(y2 * xsize + x2) * 4 + 2];
					a += mem[(y2 * xsize + x2) * 4 + 3];
				}
			}
			bm.mem[(y * bm.xsize + x) * 4 + 0] = r / ((ex - sx) * (ey - sy));
			bm.mem[(y * bm.xsize + x) * 4 + 1] = g / ((ex - sx) * (ey - sy));
			bm.mem[(y * bm.xsize + x) * 4 + 2] = b / ((ex - sx) * (ey - sy));
			bm.mem[(y * bm.xsize + x) * 4 + 3] = a / ((ex - sx) * (ey - sy));
		}
	}

	return bm;
}


void CBitMap::InvertColors() {
	if (type != BitmapTypeStandardRGBA) {
		return;
	}
	for (int y = 0; y < ysize; ++y) {
		for (int x = 0; x < xsize; ++x) {
			const int base = ((y * xsize) + x) * 4;
			mem[base + 0] = 0xFF - mem[base + 0];
			mem[base + 1] = 0xFF - mem[base + 1];
			mem[base + 2] = 0xFF - mem[base + 2];
			// do not invert alpha
		}
	}
}


void CBitMap::GrayScale() {
	if (type != BitmapTypeStandardRGBA) {
		return;
	}
	for (int y = 0; y < ysize; ++y) {
		for (int x = 0; x < xsize; ++x) {
			const int base = ((y * xsize) + x) * 4;
			const float illum =
				(mem[base + 0] * 0.299f) +
				(mem[base + 1] * 0.587f) +
				(mem[base + 2] * 0.114f);
			const unsigned int  ival = (unsigned int)(illum * (256.0f / 255.0f));
			const unsigned char cval = (ival <= 0xFF)? ival: 0xFF;
			mem[base + 0] = cval;
			mem[base + 1] = cval;
			mem[base + 2] = cval;
		}
	}
}

static ILubyte TintByte(ILubyte value, float tint) {
	float f = (float)value;
	f = std::max(0.0f, std::min(255.0f, f * tint));
	return (unsigned char) f;
}


void CBitMap::Tint(const float tint[3]) {
	if (type != BitmapTypeStandardRGBA) {
		return;
	}
	for (int y = 0; y < ysize; y++) {
		for (int x = 0; x < xsize; x++) {
			const int base = ((y * xsize) + x) * 4;
			mem[base + 0] = TintByte(mem[base + 0], tint[0]);
			mem[base + 1] = TintByte(mem[base + 1], tint[1]);
			mem[base + 2] = TintByte(mem[base + 2], tint[2]);
			// don't touch the alpha channel
		}
	}
}


void CBitMap::ReverseYAxis() {
	unsigned char* buf = new unsigned char[xsize * ysize * 4];

	for (int y = 0; y < ysize; ++y) {
		for (int x = 0; x < xsize; ++x) {
			buf[((ysize - 1 - y) * xsize + x) * 4 + 0] = mem[((y) * xsize + x) * 4 + 0];
			buf[((ysize - 1 - y) * xsize + x) * 4 + 1] = mem[((y) * xsize + x) * 4 + 1];
			buf[((ysize - 1 - y) * xsize + x) * 4 + 2] = mem[((y) * xsize + x) * 4 + 2];
			buf[((ysize - 1 - y) * xsize + x) * 4 + 3] = mem[((y) * xsize + x) * 4 + 3];
		}
	}

	delete[] mem;
	mem = buf;
}
