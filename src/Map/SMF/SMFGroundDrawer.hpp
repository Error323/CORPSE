#ifndef PFFG_SMFGROUNDDRAWER_HDR
#define PFFG_SMFGROUNDDRAWER_HDR

#include <vector>
#include "../BaseGroundDrawer.hpp"
#include "../../Math/vec3fwd.hpp"

class VertexArray;
struct Camera;
class CSMFReadMap;
class CSMFGroundTextures;

class CSMFGroundDrawer: public CBaseGroundDrawer {
	public:
		CSMFGroundDrawer(CSMFReadMap*);
		~CSMFGroundDrawer(void);

		void Draw(const Camera*, bool);

	protected:
		int viewRadius;

		CSMFReadMap* map;
		CSMFGroundTextures* textures;
		VertexArray* va;

		const int bigSquareSize;
		const int numBigTexX;
		const int numBigTexY;
		const int maxIdx;

		int mapWidth;
		int bigTexH;
		int neededLod;

		float* heightData;
		const int heightDataX;

		struct fline {
			float base;
			float dir;
		};
		std::vector<fline> right, left;

		friend class CSMFReadMap;

		void FindRange(int&, int&, std::vector<fline>&, std::vector<fline>&, int, int);
		void DoDrawGroundRow(const Camera*, int, bool);
		void AddVertex(int, int, bool);
		void AddVertex(int, int, float, bool);

		inline bool BigTexSquareRowVisible(const Camera*, int);
		void SetupTextureUnits(const Camera*, bool, unsigned int);
		void ResetTextureUnits(bool, unsigned int);

		void AddFrustumRestraint(const vec3f&, const vec3f&);
		void UpdateCamRestraints(const Camera*);
};

#endif
