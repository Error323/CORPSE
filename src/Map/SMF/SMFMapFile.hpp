#ifndef PFFG_SMFMAPFILE_HDR
#define PFFG_SMFMAPFILE_HDR

#include <string>
#include <vector>

#include "../../System/FileHandler.hpp"
#include "../ReadMap.hpp"
#include "./SMFFormat.hpp"

class CSMFMapFile {
	public:
		CSMFMapFile(const std::string& mapname);

		void ReadMinimap(void* data);
		void ReadHeightmap(unsigned short* heightmap);
		void ReadHeightmap(float* heightmap, float base, float mod);
		void ReadFeatureInfo();
		void ReadFeatureInfo(MapFeatureInfo* f);
		MapBitmapInfo GetInfoMapSize(const std::string& name) const;
		bool ReadInfoMap(const std::string& name, void* data);

		int GetNumFeatures()     const { return featureHeader.numFeatures; }
		int GetNumFeatureTypes() const { return featureHeader.numFeatureType; }

		const char* GetFeatureType(int typeID) const;

		const SMFHeader& GetHeader() const { return header; }
		CFileHandler ifs;

	private:
		void ReadGrassMap(void* data);

		SMFHeader header;

		MapFeatureHeader featureHeader;
		std::vector<std::string> featureTypes;
		int featureFileOffset;
};

#endif
