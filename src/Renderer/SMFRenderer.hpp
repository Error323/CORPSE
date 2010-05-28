#ifndef SMF_RENDERER_HDR
#define SMF_RENDERER_HDR

#include "./GLObjects.hpp"

class CReadMap;
struct Camera;

class CSMFRenderer {
	public:
		CSMFRenderer();
		~CSMFRenderer();

		struct Square {
			// note: store neighbor information explicitely?
			Square(): tlp(NVECf), trp(NVECf), brp(NVECf), blp(NVECf), mid(NVECf), idx(-1), hasDisLst(false), disLstID(0) {}

			vec3f GetMinBounds() const { return vec3f(tlp.x, -1000.0f, tlp.z); }
			vec3f GetMaxBounds() const { return vec3f(brp.x,  1000.0f, brp.z); }

			vec3f tlp, trp;
			vec3f brp, blp;
			vec3f mid;
			int idx;

			bool hasDisLst;
			unsigned int disLstID;
		};

		void BuildSquaresArray();
		void GetPotentiallyVisibleSquares(const vec3f&, const vec3f&, int*, int*);
		void DrawPotentiallyVisibleSquares(const Camera*, int, int);
		bool SquareRowVisible(int, const CReadMap*, const Camera*);
		bool SquareColVisible(int, const CReadMap*, const Camera*);
		void DrawSquare(Square&, const vec3f&, const CReadMap*, const float*);
		void Render(const Camera*);

		int viewRadius, viewRadiusSq;
		int squareSizeX, squareSizeZ;
		int numSquaresX, numSquaresZ;
		int numSquares;
		std::vector<Square> squares;
		std::vector<int> lodDists;
		std::vector<int> indices;
		GL::VertexArray va;
};

#endif
