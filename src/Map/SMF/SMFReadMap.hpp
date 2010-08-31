#ifndef PFFG_SMFREADMAP_HDR
#define PFFG_SMFREADMAP_HDR

#include "./SMFMapFile.hpp"
#include "./SMFGroundDrawer.hpp"

class CSMFReadMap: public CReadMap {
	public:
		CSMFReadMap(std::string mapname);
		~CSMFReadMap();

		void HeightmapUpdated(int x1, int x2, int y1, int y2);
		/*GLu*/ unsigned int GetShadingTexture() { return shadingTex; }
		/*GLu*/ unsigned int GetGrassShadingTexture() { return minimapTex; }
		void DrawMinimap() { /* empty */ }
		//// void GridVisibility(CCamera* cam, int quadSize, float maxdist, IQuadDrawer* cb, int extraSize);

		CBaseGroundDrawer* GetGroundDrawer() { return groundDrawer; }

		const float* GetHeightmap() const { return heightmap; }

		inline void SetHeight(int idx, float h) {
			heightmap[idx] = h;
			currMinHeight = std::min(h, currMinHeight);
			currMaxHeight = std::max(h, currMaxHeight);
		}
		inline void AddHeight(int idx, float a) {
			heightmap[idx] += a;
			currMinHeight = std::min(heightmap[idx], currMinHeight);
			currMaxHeight = std::max(heightmap[idx], currMaxHeight);
		}

		int GetNumFeatureTypes();
		int GetNumFeatures();
		// returns all feature info in MapFeatureInfo[NumFeatures]
		void GetFeatureInfo(MapFeatureInfo* f);
		const char* GetFeatureType(int typeID);

		// unsigned char* GetInfoMap(const std::string& name, MapBitmapInfo* bm);
		// void FreeInfoMap(const std::string& name, unsigned char* data);

		// todo: do not use, just here for backward compatibility with SMFGroundTextures.cpp
		CSMFMapFile& GetFile() { return smfMapFile; }

		bool usePBO;
		float anisotropy;

	protected:
		void ConfigureAnisotropy();

		CSMFMapFile smfMapFile;

		unsigned char waterHeightColors[1024 * 4];

		std::string detailTexName;
		/*GLu*/ unsigned int detailTex;
		/*GLu*/ unsigned int shadingTex;
		/*GLu*/ unsigned int minimapTex;

		float* heightmap;

		friend class CSMFGroundDrawer;
		CSMFGroundDrawer* groundDrawer;

		vec3f GetLightValue(int x, int y);
		void ParseSMD(std::string filename);
};

#endif
