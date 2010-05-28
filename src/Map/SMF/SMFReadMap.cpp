#include <GL/glew.h>
#include <GL/gl.h>

#include "../../Math/BitOps.hpp"
#include "../../System/EngineAux.hpp"
#include "../../System/Logger.hpp"
#include "./SMFReadMap.hpp"
#include "./SMFFormat.hpp"
#include "../MapInfo.hpp"
//// #include "FileSystem/FileHandler.h"
//// #include "./SMFGroundTextures.hpp"
#include "./SMFGroundDrawer.hpp"
//// #include "Rendering/Textures/Bitmap.h"
//// #include "Game/Camera.h"

using namespace std;

CSMFReadMap::CSMFReadMap(std::string mapname): smfMapFile(mapname) {
	LOG << "[CSMFReadMap::CSMFReadMap] [1]\n";

	ConfigureAnisotropy();
	usePBO = 1;

	for (int a = 0; a < 1024; ++a) {
		for (int b = 0; b < 3; ++b) {
			float c = max(mapInfo->water.minColor.y, mapInfo->water.baseColor.y - mapInfo->water.absorb.y * a);
			waterHeightColors[a * 4 + b] = (unsigned char)(c * 210);
		}
		waterHeightColors[a * 4 + 3] = 1;
	}

	LOG << "[CSMFReadMap::CSMFReadMap] [2]\n";
	const SMFHeader& header = smfMapFile.GetHeader();

	width      = header.mapx;
	height     = header.mapy;
	mapx       = header.mapx;
	mapy       = header.mapy;
	mapSquares = mapx * mapy;
	hmapx      = mapx / 2;
	hmapy      = mapy / 2;
	pwr2mapx   = next_power_of_2(mapx);
	pwr2mapy   = next_power_of_2(mapy);

	LOG << "[CSMFReadMap::CSMFReadMap] [2]\n";
	maxxpos = mapx * SQUARE_SIZE - 1;
	maxzpos = mapy * SQUARE_SIZE - 1;
	heightmap = new float[(mapx + 1) * (mapy + 1)];

	LOG << "\twidth,    height:   " << width    << ", " << height   << "\n";
	LOG << "\tmapx,     mapy:     " << mapx     << ", " << mapy     << "\n";
	LOG << "\thmapx,    hmapy:    " << hmapx    << ", " << hmapy    << "\n";
	LOG << "\tpwr2mapx, pwr2mapy: " << pwr2mapx << ", " << pwr2mapy << "\n";
	LOG << "\tmaxxpos,  maxzpos:  " << maxxpos  << ", " << maxzpos  << "\n";
	LOG << "\theightmap size ((mapx + 1) * (mapy + 1)): " << ((mapx + 1) * (mapy + 1)) << "\n";
	LOG << "\tmapSquares     ((mapx    ) * (mapy    )): " << mapSquares << "\n";

	const CMapInfo::smf_t& smf = mapInfo->smf;
	const float minH = smf.minHeightOverride? smf.minHeight: header.minHeight;
	const float maxH = smf.maxHeightOverride? smf.maxHeight: header.maxHeight;

	const float base = minH;
	const float mod = (maxH - minH) / 65536.0f;

	LOG << "[CSMFReadMap::CSMFReadMap] [3]\n";
	smfMapFile.ReadHeightmap(heightmap, base, mod);

	CReadMap::Initialize();

	for (unsigned int a = 0; a < mapname.size(); ++a) {
		mapChecksum += mapname[a];
		mapChecksum *= mapname[a];
	}



	/*
	detailTexName = mapInfo->smf.detailTexName;

	CBitmap bm;
	if (!bm.Load(detailTexName)) {
		throw content_error("Could not load detail texture from file " + detailTexName);
	}

	glGenTextures(1, &detailTex);
	glBindTexture(GL_TEXTURE_2D, detailTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glBuildMipmaps(GL_TEXTURE_2D,GL_RGBA8, bm.xsize, bm.ysize, GL_RGBA, GL_UNSIGNED_BYTE, bm.mem);
	*/

	if (anisotropy != 0.0f) {
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
	}

	unsigned char* buf = new unsigned char[MINIMAP_SIZE];
	smfMapFile.ReadMinimap(buf);

	LOG << "[CSMFReadMap::CSMFReadMap] [4]\n";

	/*
	glGenTextures(1, &minimapTex);
	glBindTexture(GL_TEXTURE_2D, minimapTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	// glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 512, 512, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);

	int offset = 0;
	for (unsigned int i = 0; i < MINIMAP_NUM_MIPMAP; i++) {
		int mipsize = 1024 >> i;
		int size = ((mipsize + 3) / 4) * ((mipsize + 3) / 4) * 8;

		glCompressedTexImage2DARB(GL_TEXTURE_2D, i, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, mipsize, mipsize, 0, size, buf + offset);
		offset += size;
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MINIMAP_NUM_MIPMAP - 1);
	*/
	delete[] buf;

	LOG << "[CSMFReadMap::CSMFReadMap] [5]\n";



	glGenTextures(1, &shadingTex);
	glBindTexture(GL_TEXTURE_2D, shadingTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, pwr2mapx, pwr2mapy, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	if (anisotropy != 0.0f) {
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
	}



	HeightmapUpdated(0, mapx, 0, mapy);
	groundDrawer = new CSMFGroundDrawer(this);

	LOG << "[CSMFReadMap::CSMFReadMap] [6]\n";
	LOG << "\tgroundDrawer instance: " << groundDrawer << "\n";
	smfMapFile.ReadFeatureInfo();
	LOG << "[CSMFReadMap::CSMFReadMap] [7]\n";
}


CSMFReadMap::~CSMFReadMap() {
	delete groundDrawer; groundDrawer = 0x0;
	delete[] heightmap;

	if (detailTex) glDeleteTextures (1, &detailTex);
	if (minimapTex) glDeleteTextures (1, &minimapTex);
	if (shadingTex) glDeleteTextures (1, &shadingTex);
}


void CSMFReadMap::HeightmapUpdated(int x1, int x2, int y1, int y2) {
	LOG << "[CSMFReadMap::HeightmapUpdated] [1]\n";
	LOG << "\tx1, x2: " << x1 << ", " << x2 << "\n";
	LOG << "\ty1, y2: " << y1 << ", " << y2 << "\n";
	// note: what the hell are these magic values?
	x1 -= x1 & 3;
	x2 += (20004 - x2) & 3;

	y1 -= y1 & 3;
	y2 += (20004 - y2) & 3;

	int xsize = x2 - x1;
	int ysize = y2 - y1;

	unsigned char* tempMem = new unsigned char[xsize * ysize * 4];

	for (int y = 0; y < ysize; ++y) {
		for (int x = 0; x < xsize; ++x) {
			float height = centerheightmap[(x + x1) + (y + y1) * mapx];

			if (height < 0) {
				int h = (int) -height;

				if (height > -10) {
					vec3f light = GetLightValue(x + x1, y + y1) * 210.0f;
					float wc = -height * 0.1f;
					tempMem[(y * xsize + x) * 4 + 0] = (unsigned char) (waterHeightColors[h * 4 + 0] * wc + light.x * (1 - wc));
					tempMem[(y * xsize + x) * 4 + 1] = (unsigned char) (waterHeightColors[h * 4 + 1] * wc + light.y * (1 - wc));
					tempMem[(y * xsize + x) * 4 + 2] = (unsigned char) (waterHeightColors[h * 4 + 2] * wc + light.z * (1 - wc));
				} else if (h < 1024) {
					tempMem[(y * xsize + x) * 4 + 0] = waterHeightColors[h * 4 + 0];
					tempMem[(y * xsize + x) * 4 + 1] = waterHeightColors[h * 4 + 1];
					tempMem[(y * xsize + x) * 4 + 2] = waterHeightColors[h * 4 + 2];
				} else {
					tempMem[(y * xsize + x) * 4 + 0] = waterHeightColors[1023 * 4 + 0];
					tempMem[(y * xsize + x) * 4 + 1] = waterHeightColors[1023 * 4 + 1];
					tempMem[(y * xsize + x) * 4 + 2] = waterHeightColors[1023 * 4 + 2];
				}
				tempMem[(y * xsize + x) * 4 + 3] = EncodeHeight(height);
			} else {
				vec3f light = GetLightValue(x + x1, y + y1) * 210.0f;
				tempMem[(y * xsize + x) * 4    ] = (unsigned char) light.x;
				tempMem[(y * xsize + x) * 4 + 1] = (unsigned char) light.y;
				tempMem[(y * xsize + x) * 4 + 2] = (unsigned char) light.z;
				tempMem[(y * xsize + x) * 4 + 3] = 255;
			}
		}
	}

	LOG << "[CSMFReadMap::HeightmapUpdated] [2]\n";

	// generate the shading texture (no per-vertex normals)
	glBindTexture(GL_TEXTURE_2D, shadingTex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, x1, y1, xsize, ysize, GL_RGBA, GL_UNSIGNED_BYTE, tempMem);

	delete[] tempMem;

}


vec3f CSMFReadMap::GetLightValue(int x, int y) {
	vec3f n1 =
		facenormals[((y * mapx) + x) * 2    ] +
		facenormals[((y * mapx) + x) * 2 + 1];
	vec3f light =
		mapInfo->light.groundDiffuseColor *
		mapInfo->light.sunDir.dot3D(n1.inorm());

	light.x = std::max(light.x, 0.0f);
	light.y = std::max(light.y, 0.0f);
	light.z = std::max(light.z, 0.0f);

	light += mapInfo->light.groundAmbientColor;

	light.x = std::min(light.x, 1.0f);
	light.y = std::min(light.y, 1.0f);
	light.z = std::min(light.z, 1.0f);

	return light;
}


// determine the map "quads" that are within the camera frustrum (?)
// NOTE: almost exactly equal to parts of CSMFGroundDrawer
/*
void CSMFReadMap::GridVisibility(CCamera* cam, int quadSize, float maxdist, CReadMap::IQuadDrawer* qd, int extraSize) {
	const SMFHeader& header = smfMapFile.GetHeader();

	int cx = int(cam->pos.x / (SQUARE_SIZE * quadSize));
	int cy = int(cam->pos.z / (SQUARE_SIZE * quadSize));

	int drawSquare = int(maxdist / (SQUARE_SIZE * quadSize)) + 1;
	int drawQuadsX = header.mapx / quadSize;
	int sy = cy - drawSquare;
	int ey = cy + drawSquare;

	if (sy < 0)
		sy = 0;
	if (ey >= header.mapy / quadSize)
		ey = header.mapy/quadSize - 1;

	int sxi = cx - drawSquare;
	int exi = cx + drawSquare;

	if (sxi < 0)
		sxi = 0;
	if (exi > drawQuadsX - 1)
		exi = drawQuadsX - 1;

	for (int y = sy; y <= ey; y++) {
		int sx = sxi;
		int ex = exi;
		float xtest, xtest2;
		std::vector<CSMFGroundDrawer::fline>::iterator fli;

		for (fli = groundDrawer->left.begin(); fli != groundDrawer->left.end(); fli++) {
			xtest = ((fli->base / SQUARE_SIZE + fli->dir * (y * quadSize)));
			xtest2 = ((fli->base / SQUARE_SIZE + fli->dir * ((y * quadSize) + quadSize)));

			if (xtest > xtest2) xtest = xtest2;
			xtest = xtest / quadSize;
			if (xtest - extraSize > sx) sx = ((int) xtest) - extraSize;
		}

		for (fli = groundDrawer->right.begin(); fli != groundDrawer->right.end(); fli++) {
			xtest = ((fli->base / SQUARE_SIZE + fli->dir * (y * quadSize)));
			xtest2 = ((fli->base / SQUARE_SIZE + fli->dir * ((y * quadSize) + quadSize)));

			if (xtest < xtest2) xtest = xtest2;
			xtest = xtest / quadSize;
			if (xtest + extraSize < ex) ex = ((int) xtest) + extraSize;
		}

		for (int x = sx; x <= ex; x++)
			qd->DrawQuad(x, y);
	}
}
*/

int CSMFReadMap::GetNumFeatures() {
	return smfMapFile.GetNumFeatures();
}

int CSMFReadMap::GetNumFeatureTypes() {
	return smfMapFile.GetNumFeatureTypes();
}

void CSMFReadMap::GetFeatureInfo(MapFeatureInfo* f) {
	smfMapFile.ReadFeatureInfo(f);
}

const char* CSMFReadMap::GetFeatureType(int typeID) {
	return smfMapFile.GetFeatureType(typeID);
}



unsigned char* CSMFReadMap::GetInfoMap(const std::string& name, MapBitmapInfo* bmInfo) {
	// get size
	*bmInfo = smfMapFile.GetInfoMapSize(name);
	if (bmInfo->width <= 0) return NULL;

	// get data
	unsigned char* data = new unsigned char[bmInfo->width * bmInfo->height];
	smfMapFile.ReadInfoMap(name, data);
	return data;
}

void CSMFReadMap::FreeInfoMap(const std::string& /*name*/, unsigned char* data) {
	delete[] data;
}



void CSMFReadMap::ConfigureAnisotropy() {
	/*
	if (!GLEW_EXT_texture_filter_anisotropic) {
		anisotropy = 0.0f;
		return;
	}

	const char* SMFTexAniso = "SMFTexAniso";
	anisotropy = atof(configHandler.GetString(SMFTexAniso, "0.0").c_str());

	if (anisotropy < 1.0f) {
		anisotropy = 0.0f; // disabled
	} else {
		GLfloat maxAniso;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);

		if (anisotropy > maxAniso) {
			anisotropy = maxAniso;
			char buf[64];
			snprintf(buf, sizeof(buf), "%f", anisotropy);
			configHandler.SetString(SMFTexAniso, buf);
		}
	}
	*/
}
