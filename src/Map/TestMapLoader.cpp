#include <iostream>

#include "./MapInfo.hpp"
#include "./ReadMap.hpp"

int main(int argc, char** argv) {
	if (argc == 2) {
		// ex. "SmallDivide.smf"
		std::string mapName(argv[1]);

		// ground = new CGround();
		const CMapInfo* mapInfo = CMapInfo::GetInstance(mapName);
		const CReadMap* readMap = CReadMap::GetInstance(mapName);

		const float* heights = readMap->GetHeightmap();
		std::vector<float>& orgHeights = readMap->orgheightmap;
		std::vector<float>& cenHeights = readMap->centerheightmap;
		std::vector<vec3f>& faceNorms = readMap->facenormals;
		std::vector<float>& faceSlopes = readMap->slopemap;

		CReadMap::FreeInstance(readMap);
		CMapInfo::FreeInstance(mapInfo);
	} else {
		std::cout << "usage: " << argv[0] << " map.smf" << std::endl;
	}

	return 0;
}
