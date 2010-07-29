#include <GL/glew.h>
#include <GL/gl.h>
#include <SDL/SDL_timer.h>


#include "./SMFGroundTextures.hpp"

//// #include "FileSystem/FileHandler.h"
//// #include "Game/Camera.h"
//// #include "Rendering/GL/myGL.h"
#include "./SMFFormat.hpp"
#include "./SMFReadMap.hpp"
#include "../../Math/FastMath.hpp"
#include "../../Renderer/Camera.hpp"
#include "../../System/FileHandler.hpp"
#include "../../System/EngineAux.hpp"
#include "../../System/LuaParser.hpp"
#include "../../System/Logger.hpp"
#include "../../System/Debugger.hpp"

using std::string;
using std::max;

CSMFGroundTextures::CSMFGroundTextures(CSMFReadMap* rm):
	bigSquareSize(128),
	numBigTexX(rm->mapx / bigSquareSize),
	numBigTexY(rm->mapy / bigSquareSize) {

	usePBO = false;
	map = rm;

	if (GLEW_EXT_pixel_buffer_object && rm->usePBO) {
		glGenBuffers(10, pboIDs);
		currentPBO = 0;
		usePBO = true;
	}

	// TODO: refactor and put reading code in CSMFFile
	CFileHandler& ifs = (rm->GetFile()).ifs;
	const SMFHeader* header = &rm->GetFile().GetHeader();

	ifs.Seek(header->tilesPtr);
	tileSize = header->tilesize;

	MapTileHeader tileHeader;
	READPTR_MAPTILEHEADER(tileHeader, &ifs);

	tileMap = new int[(header->mapx * header->mapy) / 16];
	tiles   = new char[tileHeader.numTiles * SMALL_TILE_SIZE];

	int curTileSize = 0;
	int curTile = 0;


	// RIK-NOTE: this means there can be more than one
	// .smt referenced per .smf, but no actual map uses
	// the option...
	LOG << "[CSMFGroundTextures::CSMFGroundTextures]\n";
	LOG << "\tnumber of tile-files: " << tileHeader.numTileFiles << "\n";

	for (int a = 0; a < tileHeader.numTileFiles; ++a) {
		ifs.Read(&curTileSize, 4);
		curTileSize = swabdword(curTileSize);

		std::string name;

		while (true) {
			char ch;
			ifs.Read(&ch, 1);
			/* char, no swab */
			if (ch == 0) {
				break;
			}

			name += ch;
		}

		LOG << "\toriginal tile-file name: " << name << ", size: " << curTileSize << "\n";

		name =
			LUA->GetRoot()->GetTblVal("general")->GetStrVal("mapsDir", "data/maps/") +
			LUA->GetRoot()->GetTblVal("map"    )->GetStrVal("smt", "map.smt");

		CFileHandler tileFile(name);

		if (!tileFile.FileExists()) {
			LOG << "\tcould not find tile-file \"" << name << "\"\n";

			memset(&tiles[curTile * SMALL_TILE_SIZE], 0xaa, curTileSize * SMALL_TILE_SIZE);
			curTile += curTileSize;
			continue;
		}

		TileFileHeader tfh;
		READ_TILEFILEHEADER(tfh, tileFile);

		if (strcmp(tfh.magic, "spring tilefile") != 0 || tfh.version != 1 || tfh.tileSize != 32 || tfh.compressionType != 1) {
			LOG << "\tcould not open tile-file \"" << name << "\"\n";
			PFFG_ASSERT(false);
		}

		for (int b = 0; b < curTileSize; ++b) {
			tileFile.Read(&tiles[curTile * SMALL_TILE_SIZE], SMALL_TILE_SIZE);
			curTile++;
		}
	}

	int count = (header->mapx * header->mapy) / 16;
	ifs.Read(tileMap, count * sizeof(int));

	for (int i = 0; i < count; i++) {
		tileMap[i] = swabdword(tileMap[i]);
	}

	LOG << "\theader->mapx: " << header->mapx << "\n";
	LOG << "\theader->mapy: " << header->mapy << "\n";
	LOG << "\tnumBigTexX:   " << numBigTexX << "\n";
	LOG << "\tnumBigTexY:   " << numBigTexY << "\n";
	LOG << "\ttile-count:   " << count << "\n";

	tileMapXSize = header->mapx / 4;
	tileMapYSize = header->mapy / 4;

	squares = new GroundSquare[numBigTexX * numBigTexY];

	for (int y = 0; y < numBigTexY; ++y) {
		for (int x = 0; x < numBigTexX; ++x) {
			GroundSquare* square = &squares[y * numBigTexX + x];
				square->texLevel = 1;
				square->lastUsed = 0;
			LoadSquare(x, y, 2);
		}
	}
}

CSMFGroundTextures::~CSMFGroundTextures(void) {
	for (int i = 0; i < numBigTexX * numBigTexY; ++i) {
		glDeleteTextures(1, &squares[i].textureID);
	}

	delete[] squares;
	delete[] tileMap;
	delete[] tiles;

	if (usePBO) {
		glDeleteBuffers(10, pboIDs);
	}
}


void CSMFGroundTextures::SetTexture(int x, int y) {
	GroundSquare* square = &squares[y * numBigTexX + x];
	square->lastUsed = SDL_GetTicks();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, square->textureID);
}

inline bool CSMFGroundTextures::TexSquareInView(const Camera* cam, int btx, int bty) {
	const float* heightData = map->GetHeightmap();
	static const int heightDataX = map->mapx + 1;
	static const int bigTexW = (map->mapx << 3) / numBigTexX;
	static const int bigTexH = (map->mapy << 3) / numBigTexY;
	static const float bigTexSquareRadius = fastmath::sqrt1(float(bigTexW * bigTexW + bigTexH * bigTexH));

	const int x = btx * bigTexW + (bigTexW >> 1);
	const int y = bty * bigTexH + (bigTexH >> 1);
	const int idx = (y >> 3) * heightDataX + (x >> 3);
	const vec3f bigTexSquarePos(x, heightData[idx], y);

	return (cam->InView(bigTexSquarePos, bigTexSquareRadius));
}

void CSMFGroundTextures::DrawUpdate(const Camera* cam) {
	for (int y = 0; y < numBigTexY; ++y) {
		float dy = 0.0f;
			dy = cam->pos.z - y * bigSquareSize * map->SQUARE_SIZE - (map->SQUARE_SIZE << 6);
			dy = max(0.0f, float(fabs(dy) - (map->SQUARE_SIZE << 6)));

		for (int x = 0; x < numBigTexX; ++x) {
			if (!TexSquareInView(cam, x, y)) {
				// no need to update this square's
				// texture if we can't even see it
				continue;
			}

			GroundSquare* square = &squares[y * numBigTexX + x];

			float dx = 0.0f;
				dx = cam->pos.x - x * bigSquareSize * map->SQUARE_SIZE - (map->SQUARE_SIZE << 6);
				dx = max(0.0f, float(fabs(dx) - (map->SQUARE_SIZE << 6)));
			float dist = fastmath::sqrt1(dx * dx + dy * dy);

			if (square->lastUsed < (SDL_GetTicks() - 60 * 1000)) {
				dist = 8000.0f;
			}

			float wantedLevel = dist * 0.001f;

			if (wantedLevel > 2.5f)
				wantedLevel = 2.5f;
			if (wantedLevel < square->texLevel - 1)
				wantedLevel = square->texLevel - 1;

			if (square->texLevel != (int) wantedLevel) {
				glDeleteTextures(1, &square->textureID);
				LoadSquare(x, y, (int) wantedLevel);
			}
		}
	}
}



void CSMFGroundTextures::LoadSquare(int x, int y, int level) {
	LOG << "[CSMFGroundTextures::LoadSquare]\n";
	LOG << "\t  x,   y: " <<     x << ", " <<                   y  << "\n";
	LOG << "\tlvl, idx: " << level << ", " << (y * numBigTexX + x) << "\n";
	LOG << "\tusePBO, currentPBO: " << usePBO << ", " << currentPBO << "\n";

	static const int tileoffset[] = {0, 512, 640, 672};

	const int size = 1024 >> level;
	const int numblocks = 8 / (1 << level);

	GLubyte* buf = NULL;
	bool usedPBO = false;

	if (usePBO) {
		if (currentPBO > 9) {
			currentPBO = 0;
		}
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIDs[currentPBO++]);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, size * size / 2, 0, GL_STREAM_DRAW);

		// map the buffer object into client's memory
		buf = (GLubyte*) glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
		usedPBO = true;
	}

	if (buf == NULL) {
		buf = new GLubyte[size * size / 2];
		usedPBO = false;
	}

	GroundSquare* square = &squares[y * numBigTexX + x];
	square->texLevel = level;

	for (int y1 = 0; y1 < 32; y1++) {
		for (int x1 = 0; x1 < 32; x1++) {
			const int   idx  = tileMap[(x1 + x * 32) + (y1 + y * 32) * tileMapXSize];
			const char* tile = &tiles[idx * SMALL_TILE_SIZE + tileoffset[level]];

			for (int yt = 0; yt < numblocks; yt++) {
				for (int xt = 0; xt < numblocks; xt++) {
					GLint* sbuf = (GLint*) &tile[(xt + yt * numblocks) * 8];
					GLint* dbuf = (GLint*) &buf[(x1 * numblocks + xt + (y1 * numblocks + yt) * (numblocks * 32)) * 8];

					// copy 2x4 bytes at once
					dbuf[0] = sbuf[0];
					dbuf[1] = sbuf[1];
				}
			}
		}
	}

	glGenTextures(1, &square->textureID);
	glBindTexture(GL_TEXTURE_2D, square->textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_PRIORITY, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	if (GLEW_EXT_texture_edge_clamp) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	if (map->anisotropy != 0.0f) {
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, map->anisotropy);
	}

	if (usedPBO) {
		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
		glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, size, size, 0, size * size / 2, 0);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, 0, 0, GL_STREAM_DRAW); //free it
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	} else {
		glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, size, size, 0, size * size / 2, buf);
		delete[] buf;
	}

	LOG << "\tsquare->textureID: " << square->textureID << "\n";
}
