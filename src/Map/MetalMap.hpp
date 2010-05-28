#ifndef PFFG_METALMAP_HDR
#define PFFG_METALMAP_HDR

#include <vector>

class CMetalMap {
	public:
		CMetalMap(unsigned char* map, int sizeX, int sizeZ, float metalScale, int sqSize);
		virtual ~CMetalMap(void);

		float getMetalAmount(int x1, int z1, int x2, int z2);
		float getMetalAmount(int x, int z);
		float requestExtraction(int x, int z, float toDepth);
		void  removeExtraction(int x, int z, float depth);

		unsigned char* metalMap;
		unsigned char metalPal[768];
		std::vector<float> extractionMap;

		int GetSizeX() const { return sizeX; }
		int GetSizeZ() const { return sizeZ; }

		// squares on metalmap are 2x2 squares on normal map
		const float METAL_MAP_SQUARE_SIZE;

	protected:
		int sizeX;
		int sizeZ;
		float metalScale;
};

#endif
