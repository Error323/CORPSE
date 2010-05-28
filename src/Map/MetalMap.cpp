#include "./MetalMap.hpp"
#include "./ReadMap.hpp"
#include "../System/EngineAux.hpp"
#include "../System/Logger.hpp"

//// #include "Platform/ConfigHandler.h"

CMetalMap::CMetalMap(unsigned char* map, int _sizeX, int _sizeZ, float _metalScale, int sqSize):
	metalMap(map), METAL_MAP_SQUARE_SIZE(sqSize * 2),
	sizeX(_sizeX), sizeZ(_sizeZ), metalScale(_metalScale) {

	LOG << "[CMetalMap::CMetalMap] [1]\n";
	extractionMap.resize(sizeX * sizeZ, 0.0f);

	//// int whichPalette = configHandler.GetInt("MetalMapPalette", 0);
	int whichPalette = 1;

	if (whichPalette == 1) {
		/* Swap the green and blue channels. making metal go
		   black -> blue -> cyan,
		   rather than the usual black -> green -> cyan. */
		for (int a = 0; a < 256; ++a) {
			metalPal[a * 3 + 0] = a;
			metalPal[a * 3 + 1] = std::max(0, a * 2 - 255);
			metalPal[a * 3 + 2] = std::min(255, a * 2);
		}
	} else {
		for(int a = 0; a < 256; ++a) {
			metalPal[a * 3 + 0] = a;
			metalPal[a * 3 + 1] = std::min(255, a * 2);
			metalPal[a * 3 + 2] = std::max(0, a * 2 - 255);
		}
	}

	LOG << "[CMetalMap::CMetalMap] [2]\n";
}


CMetalMap::~CMetalMap(void) {
	delete[] metalMap;
}


static inline void ClampInt(int& var, int min, int maxPlusOne) {
	if (var < min) {
		var = min;
	} else if (var >= maxPlusOne) {
		var = maxPlusOne - 1;
	}
}


// amount of metal over an area (of squares)
float CMetalMap::getMetalAmount(int x1, int z1, int x2, int z2) {
	ClampInt(x1, 0, sizeX);
	ClampInt(x2, 0, sizeX);
	ClampInt(z1, 0, sizeZ);
	ClampInt(z2, 0, sizeZ);
	
	float metal = 0.0f;
	int x, z;
	for (x = x1; x < x2; x++) {
		for (z = z1; z < z2; z++) {
			metal += metalMap[(z * sizeX) + x];
		}
	}
	return metal * metalScale;
}

// amount of metal over a single square
float CMetalMap::getMetalAmount(int x, int z) {
	ClampInt(x, 0, sizeX);
	ClampInt(z, 0, sizeZ);

	return metalMap[(z * sizeX) + x] * metalScale;
}


/*
 * Makes a reqest for extracting metal from a given square.
 * If there is metal left to extract to the requested depth,
 * the amount available will be returned and the requested
 * depth will be set as new extraction-depth on the extraction-map.
 * If the requested depth is grounder than the current
 * extraction-depth 0.0 will be returned and nothing changed.
 */
float CMetalMap::requestExtraction(int x, int z, float toDepth) {
	ClampInt(x, 0, sizeX);
	ClampInt(z, 0, sizeZ);

	const float current = extractionMap[(z * sizeX) + x];

	if (toDepth <= current) {
		return 0.0f;
	}

	const float available = toDepth - current;
	extractionMap[(z * sizeX) + x] = toDepth;

	return available;
}


/*
 * When a extraction ends, the digged depth should be left
 * back to the extraction-map to be available for other
 * extractors to use.
 */
void CMetalMap::removeExtraction(int x, int z, float depth) {
	ClampInt(x, 0, sizeX);
	ClampInt(z, 0, sizeZ);

	extractionMap[(z * sizeX) + x] -= depth;
}
