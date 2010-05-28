#include <cassert>
#include "./SMFMapFile.hpp"
#include "./SMFFormat.hpp"
#include "../../System/EngineAux.hpp"
#include "../../System/Logger.hpp"

using std::string;


CSMFMapFile::CSMFMapFile(const string& mapname): ifs(mapname), featureFileOffset(0) {
	memset(&header, 0, sizeof(header));
	memset(&featureHeader, 0, sizeof(featureHeader));

	if (!ifs.FileExists()) {
		LOG << "[CSMFMapFile::CSMFMapFile]\n";
		LOG << "\tCouldn't open map file " << mapname << "\n";
		assert(false);
	}

	READPTR_MAPHEADER(header, (&ifs));

	if (strcmp(header.magic, "spring map file") != 0 ||
		header.version != 1 || header.tilesize != 32 ||
		header.texelPerSquare != 8 || header.squareSize != 8) {
		LOG << "[CSMFMapFile::CSMFMapFile]\n";
		LOG << "\tincorrect or corrupt map file " << mapname << "\n";
		assert(false);
	}
}


void CSMFMapFile::ReadMinimap(void* data) {
	ifs.Seek(header.minimapPtr);
	ifs.Read(data, MINIMAP_SIZE);
}


void CSMFMapFile::ReadHeightmap(unsigned short* heightmap) {
	const int hmx = header.mapx + 1;
	const int hmy = header.mapy + 1;

	ifs.Seek(header.heightmapPtr);
	ifs.Read(heightmap, hmx * hmy * sizeof(short));

	for (int y = 0; y < hmx * hmy; ++y) {
		heightmap[y] = swabword(heightmap[y]);
	}
}


void CSMFMapFile::ReadHeightmap(float* heightmap, float base, float mod) {
	const int hmx = header.mapx + 1;
	const int hmy = header.mapy + 1;
	unsigned short* temphm = new unsigned short[hmx * hmy];

	ifs.Seek(header.heightmapPtr);
	ifs.Read(temphm, hmx * hmy * 2);

	for (int y = 0; y < hmx * hmy; ++y) {
		heightmap[y] = base + swabword(temphm[y]) * mod;
	}

	delete[] temphm;
}


void CSMFMapFile::ReadFeatureInfo() {
	ifs.Seek(header.featurePtr);
	READ_MAPFEATUREHEADER(featureHeader, (&ifs));

	featureTypes.resize(featureHeader.numFeatureType);

	for(int a = 0; a < featureHeader.numFeatureType; ++a) {
		char c;
		ifs.Read(&c, 1);
		while (c) {
			featureTypes[a] += c;
			ifs.Read(&c, 1);
		}
	}
	featureFileOffset = ifs.GetPos();
}


void CSMFMapFile::ReadFeatureInfo(MapFeatureInfo* f) {
	assert(featureFileOffset != 0);
	ifs.Seek(featureFileOffset);

	for(int a = 0; a < featureHeader.numFeatures; ++a) {
		MapFeatureStruct ffs;
		READ_MAPFEATURESTRUCT(ffs, (&ifs));

		f[a].featureType = ffs.featureType;
		f[a].pos = vec3f(ffs.xpos, ffs.ypos, ffs.zpos);
		f[a].rotation = ffs.rotation;
	}
}


const char* CSMFMapFile::GetFeatureType(int typeID) const {
	assert(typeID >= 0 && typeID < featureHeader.numFeatureType);
	return featureTypes[typeID].c_str();
}


MapBitmapInfo CSMFMapFile::GetInfoMapSize(const string& name) const {
	if (name == "height") {
		return MapBitmapInfo(header.mapx + 1, header.mapy + 1);
	}
	else if (name == "grass") {
		return MapBitmapInfo(header.mapx / 4, header.mapy / 4);
	}
	else if (name == "metal") {
		return MapBitmapInfo(header.mapx / 2, header.mapy / 2);
	}
	else if (name == "type") {
		return MapBitmapInfo(header.mapx / 2, header.mapy / 2);
	}
	return MapBitmapInfo(0, 0);
}


bool CSMFMapFile::ReadInfoMap(const string& name, void* data) {
	if (name == "height") {
		ReadHeightmap((unsigned short*)data);
		return true;
	}
	else if (name == "grass") {
		ReadGrassMap(data);
		return true;
	}
	else if(name == "metal") {
		ifs.Seek(header.metalmapPtr);
		ifs.Read(data, header.mapx / 2 * header.mapy / 2);
		return true;
	}
	else if(name == "type") {
		ifs.Seek(header.typeMapPtr);
		ifs.Read(data, header.mapx / 2 * header.mapy / 2);
		return true;
	}
	return false;
}


void CSMFMapFile::ReadGrassMap(void* data) {
	ifs.Seek(sizeof(SMFHeader));

	for (int a = 0; a < header.numExtraHeaders; ++a) {
		int size;
		ifs.Read(&size, 4);
		size = swabdword(size);

		int type;
		ifs.Read(&type, 4);
		type = swabdword(type);

		if (type == MEH_Vegetation) {
			int pos;
			ifs.Read(&pos, 4);
			pos = swabdword(pos);
			ifs.Seek(pos);
			ifs.Read(data, header.mapx / 4 * header.mapy / 4);
			/* char; no swabbing. */
			break; //we aren't interested in other extensions anyway
		}
		else {
			// assumes we can use data as scratch memory
			assert(size - 8 <= header.mapx / 4 * header.mapy / 4);
			ifs.Read(data, size - 8);
		}
	}
}
