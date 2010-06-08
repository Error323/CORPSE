#ifndef PFFG_READMAP_HDR
#define PFFG_READMAP_HDR

#include <string>
#include <vector>
#include "../Math/vec3fwd.hpp"
#include "../Math/vec3.hpp"

class CBaseGroundDrawer;


struct MapFeatureInfo {
	vec3f pos;
	int featureType;	// index to one of the strings above (?)
	float rotation;
};

struct MapBitmapInfo {
	MapBitmapInfo() {}
	MapBitmapInfo(int w, int h): width(w), height(h) {}

	int width;
	int height;
};


class CReadMap {
protected:
	// not public, use LoadMap
	CReadMap();
	virtual ~CReadMap();

	// called by implementations of CReadMap
	void Initialize();

public:
	static CReadMap* GetInstance(const std::string& mapname = "");
	static void FreeInstance(CReadMap*);

	// calculates derived heightmap information such as normals, centerheightmap and slopemap
	void CalcHeightfieldData();
	void GenerateVertexNormals();

	virtual const float* GetHeightmap() const = 0;
	// if you modify the heightmap, call HeightmapUpdated()
	virtual void SetHeight(int idx, float h) = 0;
	virtual void AddHeight(int idx, float a) = 0;

	std::vector<float> orgheightmap;
	std::vector<float> centerheightmap;
	std::vector<float> slopemap;
	std::vector<vec3f> facenormals;
	std::vector<vec3f> vertexNormals;

	// number of heightmap mipmaps, including full resolution
	static const int numHeightMipMaps = 7;
	// array of pointers to heightmap in different resolutions, mipHeightmap[0]
	// is full resolution, mipHeightmap[n+1] is half resolution of mipHeightmap[n]
	float* mipHeightmap[numHeightMipMaps];

	unsigned char* typemap;

	virtual CBaseGroundDrawer* GetGroundDrawer() { return 0; }
	virtual void HeightmapUpdated(int x1, int x2, int y1, int y2) = 0;
	virtual void Update() {};
	virtual void Explosion(float x, float y, float strength) { x = y = strength; };

	// a texture with RGB for shading and A for height
	//// virtual GLuint GetShadingTexture () = 0;
	//// virtual GLuint GetGrassShadingTexture() { return 0; }

	static inline unsigned char EncodeHeight(float h) { return std::max(0, int(255 + 10.0f * h)); }

	// draw the minimap in a quad (with extends: (0, 0) - (1, 1))
	virtual void DrawMinimap() = 0;


	// Feature creation
	virtual int GetNumFeatures() = 0;
	virtual int GetNumFeatureTypes() = 0;
	virtual void GetFeatureInfo(MapFeatureInfo* f) = 0; // returns MapFeatureInfo[GetNumFeatures()]
	virtual const char *GetFeatureType(int typeID) = 0;

	// Infomaps (such as metal map, grass map, ...), handling them with a string as type seems flexible...
	// Some map types:
	//   "metal"  -  metalmap
	//   "grass"  -  grassmap
	virtual unsigned char* GetInfoMap(const std::string& name, MapBitmapInfo* bm) = 0;
	virtual void FreeInfoMap(const std::string& name, unsigned char* data) = 0;

	// determine visibility for a rectangular grid
	struct IQuadDrawer {
		virtual ~IQuadDrawer();
		virtual void DrawQuad(int x, int y) = 0;
	};

	//// virtual void GridVisibility(const Camera*, int quadSize, float maxdist, IQuadDrawer* cb, int extraSize = 0) = 0;

	float minheight, maxheight;
	float currMinHeight, currMaxHeight;


	inline vec3f square2pos(int x, int z) { return vec3f(x * SQUARE_SIZE, centerheightmap[z * mapx + x], z * SQUARE_SIZE); }
	inline int pos2square(const vec3f& pos) { return ((int(pos.z / SQUARE_SIZE) * (mapx + 1)) + int(pos.x / SQUARE_SIZE)); }
	inline void SetPosInBounds(vec3f& pos) {
		pos.x = std::max(0.0f, std::min(float(maxxpos), pos.x));
		pos.z = std::max(0.0f, std::min(float(maxzpos), pos.z));
	}

	// copied from GS and float3; set in CSM{F, 3}ReadMap
	int mapSquares;
	int mapx, mapy;
	int hmapx, hmapy;
	int pwr2mapx, pwr2mapy;
	int maxxpos, maxzpos;
	const int SQUARE_SIZE;

	int width, height;
	unsigned int mapChecksum;
};

#define readMap (CReadMap::GetInstance())

#endif
