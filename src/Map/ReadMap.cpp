#include <cstdlib>
#include <string>
#include <cassert>

#include "../Math/vec3.hpp"
#include "../System/EngineAux.hpp"
#include "./ReadMap.hpp"
#include "./MapInfo.hpp"
#include "./MetalMap.hpp"
#include "./SMF/SMFReadMap.hpp"
//! #include "./SM3/SM3ReadMap.hpp"
#include "../System/Logger.hpp"



CReadMap* CReadMap::GetInstance(const std::string& mapname) {
	static CReadMap* rm = NULL;

	if (rm == NULL) {
		LOG << "[CReadMap::LoadMap] [1]\n";

		assert(mapname.length() >= 3);
		std::string extension = mapname.substr(mapname.length() - 3);


		if (extension == "smf") { rm = new CSMFReadMap(mapname); }
		// if (extension == "sm3") { rm = new CSM3ReadMap(mapname); }

		if (rm == NULL) {
			LOG << "[CReadMap::LoadMap] corrupted map or unsupported map format\n";
			assert(false);
		}

		LOG << "[CReadMap::LoadMap] [2]\n";

		/* Read metal map */
		MapBitmapInfo mbi;
		unsigned char* metalmap = rm->GetInfoMap("metal", &mbi);

		LOG << "[CReadMap::LoadMap] [3]\n";

		if (metalmap && mbi.width == rm->width / 2 && mbi.height == rm->height / 2) {
			int size = mbi.width * mbi.height;
			unsigned char* map = new unsigned char[size];
			memcpy(map, metalmap, size);
			rm->metalMap = new CMetalMap(map, mbi.width, mbi.height, mapInfo->map.maxMetal, rm->SQUARE_SIZE);
		}

		LOG << "[CReadMap::LoadMap] [4]\n";

		if (metalmap != NULL) {
			rm->FreeInfoMap("metal", metalmap);
		}

		if (rm->metalMap == NULL) {
			unsigned char* zd = new unsigned char[rm->width * rm->height / 4];
			memset(zd, 0, rm->width * rm->height / 4);
			rm->metalMap = new CMetalMap(zd, rm->width / 2, rm->height / 2, 1.0f, rm->SQUARE_SIZE);
		}

		LOG << "[CReadMap::LoadMap] [5]\n";

		/* Read type map */
		MapBitmapInfo tbi;
		unsigned char* typemap = rm->GetInfoMap("type", &tbi);

		LOG << "[CReadMap::LoadMap] [6]\n";

		if (typemap && tbi.width == rm->width / 2 && tbi.height == rm->height / 2) {
			assert(rm->hmapx == tbi.width && rm->hmapy == tbi.height);
			rm->typemap = new unsigned char[tbi.width * tbi.height];
			memcpy(rm->typemap, typemap, tbi.width * tbi.height);
		} else {
			LOG << "[CReadMap::LoadMap] bad or missing terrain type map\n";
			assert(false);
		}

		LOG << "[CReadMap::LoadMap] [7]\n";

		if (typemap != NULL) {
			rm->FreeInfoMap("type", typemap);
		}

		LOG << "[CReadMap::LoadMap] [8]\n";
	}

	return rm;
}

void CReadMap::FreeInstance(CReadMap* rm) {
	delete rm;
}



CReadMap::CReadMap():
	typemap(NULL), metalMap(NULL), SQUARE_SIZE(8) {
	memset(mipHeightmap, 0, sizeof(mipHeightmap));
}

CReadMap::~CReadMap() {
	delete metalMap;
	delete[] typemap;

	// don't delete first pointer since it points to centerheightmap
	for (int i = 1; i < numHeightMipMaps; i++) {
		delete[] mipHeightmap[i];
	}
}



void CReadMap::Initialize() {
	LOG << "[CReadMap::Initialize] [1]\n";
	orgheightmap.resize((mapx + 1) * (mapy + 1), 0.0f);
	centerheightmap.resize(mapx * mapy, 0.0f);
	facenormals.resize(mapx * mapy * 2, NVECf);
	slopemap.resize(hmapx * hmapy, 0.0f);

	LOG << "[CReadMap::Initialize] [2]\n";

	// first mip-hmap is reference to centerheightmap
	mipHeightmap[0] = &centerheightmap[0];

	LOG << "[CReadMap::Initialize] [3]\n";

	for (int i = 1; i < numHeightMipMaps; i++) {
		mipHeightmap[i] = new float[(mapx >> i) * (mapy >> i)];
	}

	LOG << "[CReadMap::Initialize] [4]\n";

	CalcHeightfieldData();
	GenerateVertexNormals();
}

void CReadMap::CalcHeightfieldData() {
	const float* heightmap = GetHeightmap();

	minheight = +123456.0f;
	maxheight = -123456.0f;

	mapChecksum = 0;
	for (int y = 0; y < ((mapy + 1) * (mapx + 1)); ++y) {
		orgheightmap[y] = heightmap[y];
		if (heightmap[y] < minheight) { minheight = heightmap[y]; }
		if (heightmap[y] > maxheight) { maxheight = heightmap[y]; }
		mapChecksum +=  (unsigned int) (heightmap[y] * 100);
		mapChecksum ^= *(unsigned int*) &heightmap[y];
	}

	currMinHeight = minheight;
	currMaxHeight = maxheight;

	for (int y = 0; y < (mapy); y++) {
		for (int x = 0; x < (mapx); x++) {
			float height = heightmap[(y) * (mapx + 1) + x];
			height += heightmap[(y    ) * (mapx + 1) + x + 1];
			height += heightmap[(y + 1) * (mapx + 1) + x    ];
			height += heightmap[(y + 1) * (mapx + 1) + x + 1];
			centerheightmap[y * mapx + x] = height * 0.25f;
		}
	}

	for (int i = 0; i < numHeightMipMaps - 1; i++) {
		int hmapx = mapx >> i;
		int hmapy = mapy >> i;

		for (int y = 0; y < hmapy; y += 2) {
			for (int x = 0; x < hmapx; x += 2) {
				float height = mipHeightmap[i][(x) + (y) * hmapx];
				height += mipHeightmap[i][(x    ) + (y + 1) * hmapx];
				height += mipHeightmap[i][(x + 1) + (y    ) * hmapx];
				height += mipHeightmap[i][(x + 1) + (y + 1) * hmapx];
				mipHeightmap[i + 1][(x / 2) + (y / 2) * hmapx / 2] = height * 0.25f;
			}
		}
	}

	// create the surface normals (these are used
	// to generate the ground shading texture, the
	// map renderers do not set per-vertex normals)
	for (int y = 0; y < mapy; y++) {
		for (int x = 0; x < mapx; x++) {
			int idx0 = (y    ) * (mapx + 1) + x;
			int idx1 = (y + 1) * (mapx + 1) + x;

			vec3f e1(-SQUARE_SIZE, heightmap[idx0] - heightmap[idx0 + 1],            0);
			vec3f e2(           0, heightmap[idx0] - heightmap[idx1    ], -SQUARE_SIZE);

			facenormals[(y * mapx + x) * 2] = e2.cross(e1).inorm();

			e1 = vec3f( SQUARE_SIZE, heightmap[idx1 + 1] - heightmap[idx1    ],           0);
			e2 = vec3f(           0, heightmap[idx1 + 1] - heightmap[idx0 + 1], SQUARE_SIZE);

			facenormals[(y * mapx + x) * 2 + 1] = e2.cross(e1).inorm();
		}
	}

	for (int y = 0; y < hmapy; y++) {
		for (int x = 0; x < hmapx; x++) {
			slopemap[y * hmapx + x] = 1;
		}
	}

	const int ss4 = SQUARE_SIZE * 4;
	for (int y = 2; y < mapy - 2; y += 2) {
		for (int x = 2; x < mapx - 2; x += 2) {
			int idx0 = (y - 1) * (mapx + 1);
			int idx1 = (y + 3) * (mapx + 1);

			vec3f e1(-ss4, heightmap[idx0 + (x - 1)] - heightmap[idx0 +  x + 3 ],    0);
			vec3f e2(   0, heightmap[idx0 + (x - 1)] - heightmap[idx1 + (x - 1)], -ss4);

			vec3f n = e2.cross(e1);
			n.inorm();

			e1 = vec3f(ss4, heightmap[idx1 + x + 3] - heightmap[idx1 + (x - 1)],   0);
			e2 = vec3f(  0, heightmap[idx1 + x + 3] - heightmap[idx0 +  x + 3 ], ss4);

			vec3f n2 = e2.cross(e1);
			n2.inorm();

			slopemap[(y / 2) * hmapx + (x / 2)] = 1.0f - (n.y + n2.y) * 0.5f;
		}
	}

	LOG << "[CReadMap::CalcHeightfieldData]\n";
	LOG << "\tterrain normals and slopes calculated\n";
	LOG << "\tminHeight: " << minheight << ", maxHeight: " << maxheight << "\n";
}



// note: not part of the original CReadMap, but used as
// a stand-in for the shading texture (would need to be
// integrated with SMFReadMap::HeightmapUpdated() also
// for deformable terrain)
void CReadMap::GenerateVertexNormals() {
	LOG << "[CReadMap::GenerateVertexNormals] [1]\n";

	const float* hm = GetHeightmap();
	const int W = (mapx + 1);
	const int H = (mapy + 1);
	const int sqSz = SQUARE_SIZE;

	vertexNormals.resize(W * H, NVECf);

	for (int x = 0; x <= mapx; x++) {
		for (int z = 0; z <= mapy; z++) {
			const int i = (z * W) + x;

			// don't bother with the edge vertices
			if (x == 0 || x == W - 1) { vertexNormals[i] = YVECf; continue; }
			if (z == 0 || z == H - 1) { vertexNormals[i] = YVECf; continue; }

			vec3f n = NVECf;

			const float htl = hm[((z - 1) * W) + (x - 1)]; // vertex to the top-left
			const float htm = hm[((z - 1) * W) + (x    )]; // vertex to the top-middle
			const float htr = hm[((z - 1) * W) + (x + 1)]; // vertex to the top-right

			const float hml = hm[((z    ) * W) + (x - 1)]; // vertex to the middle-left
			const float hmm = hm[((z    ) * W) + (x    )]; // the center vertex
			const float hmr = hm[((z    ) * W) + (x + 1)]; // vertex to the middle-right

			const float hbl = hm[((z + 1) * W) + (x - 1)]; // vertex to the bottom-left
			const float hbm = hm[((z + 1) * W) + (x    )]; // vertex to the bottom-middle
			const float hbr = hm[((z + 1) * W) + (x + 1)]; // vertex to the bottom-right

			// top row of vertices
			const vec3f vtl((x - 1) * sqSz, htl, (z - 1) * sqSz);
			const vec3f vtm((x    ) * sqSz, htm, (z - 1) * sqSz);
			const vec3f vtr((x + 1) * sqSz, htr, (z - 1) * sqSz);

			// middle row of vertices
			const vec3f vml((x - 1) * sqSz, hml, (z    ) * sqSz);
			const vec3f vmm((x    ) * sqSz, hmm, (z    ) * sqSz);
			const vec3f vmr((x + 1) * sqSz, hmr, (z    ) * sqSz);

			// bottom row of vertices
			const vec3f vbl((x - 1) * sqSz, hbl, (z + 1) * sqSz);
			const vec3f vbm((x    ) * sqSz, hbm, (z + 1) * sqSz);
			const vec3f vbr((x + 1) * sqSz, hbr, (z + 1) * sqSz);

			// normal vector of each virtual triangle
			// make sure the normal always points up
			vec3f tn;
				tn = (vtl - vmm).cross((vtm - vmm)); if (tn.y < 0.0f) { tn = -tn; }; n += tn;
				tn = (vtm - vmm).cross((vtr - vmm)); if (tn.y < 0.0f) { tn = -tn; }; n += tn;
				tn = (vtr - vmm).cross((vmr - vmm)); if (tn.y < 0.0f) { tn = -tn; }; n += tn;
				tn = (vmr - vmm).cross((vbr - vmm)); if (tn.y < 0.0f) { tn = -tn; }; n += tn;
				tn = (vbr - vmm).cross((vbm - vmm)); if (tn.y < 0.0f) { tn = -tn; }; n += tn;
				tn = (vbm - vmm).cross((vbl - vmm)); if (tn.y < 0.0f) { tn = -tn; }; n += tn;
				tn = (vbl - vmm).cross((vml - vmm)); if (tn.y < 0.0f) { tn = -tn; }; n += tn;
				tn = (vml - vmm).cross((vtl - vmm)); if (tn.y < 0.0f) { tn = -tn; }; n += tn;

			/*
			n += (vtl - vmm).cross((vtm - vmm)).norm();
			n += (vtm - vmm).cross((vtr - vmm)).norm();
			n += (vtr - vmm).cross((vmr - vmm)).norm();
			n += (vmr - vmm).cross((vbr - vmm)).norm();
			n += (vbr - vmm).cross((vbm - vmm)).norm();
			n += (vbm - vmm).cross((vbl - vmm)).norm();
			n += (vbl - vmm).cross((vml - vmm)).norm();
			n += (vml - vmm).cross((vtl - vmm)).norm();
			n *= -0.125f;
			n.inorm();
			*/

			vertexNormals[i] = n.norm();
		}
	}

	LOG << "[CReadMap::GenerateVertexNormals] [2]\n";
}



CReadMap::IQuadDrawer::~IQuadDrawer() {
}
