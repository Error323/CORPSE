#include <GL/glew.h>
#include <GL/gl.h>
#include "./GL.hpp"

void mglBuildMipMaps(int target, int internalFormat, int width, int height, int format, int type, const void* data) {
	if (false /*gu->compressTextures*/) {
		switch (internalFormat) {
			case 4:
			case GL_RGBA8:
			case GL_RGBA: internalFormat = GL_COMPRESSED_RGBA_ARB; break;

			case 3:
			case GL_RGB8:
			case GL_RGB: internalFormat = GL_COMPRESSED_RGB_ARB; break;

			case GL_LUMINANCE: internalFormat = GL_COMPRESSED_LUMINANCE_ARB; break;
		}
	}

	// create mipmapped texture

	/*
	if (!gu->atiHacks && glGenerateMipmapEXT) {
		// newest method, broken on ATIs and NVs
		glTexImage2D(target, 0, internalFormat, width, height, 0, format, type, data);
		glGenerateMipmapEXT(target);
	} else
	*/
	if (GLEW_VERSION_1_4) {
		// instead of using glu, we rely on glTexImage2D to create the mipmaps
		glTexParameteri(target, GL_GENERATE_MIPMAP, GL_TRUE);
		glTexImage2D(target, 0, internalFormat, width, height, 0, format, type, data);
	} else {
		gluBuild2DMipmaps(target, internalFormat, width, height, format, type, data);
	}
}
