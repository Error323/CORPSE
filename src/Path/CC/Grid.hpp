#ifndef PFFG_GRID_HDR
#define PFFG_GRID_HDR

#include <vector>
#include <map>
#include <limits>

#include "../../Math/vec3fwd.hpp"
#include "../../Math/vec3.hpp"
#include "../../Ext/ICallOutHandler.hpp"

enum {
	NORTH      = 0,
	EAST       = 1,
	SOUTH      = 2,
	WEST       = 3,
	DIRECTIONS = 4
};

class Face;
class Cell;
class Grid {
public:
	Grid() {}
	~Grid();

	vec3f Grid2Real(const Cell*);

	void Init(const int, ICallOutHandler*);
	void AddDensityAndVelocity(const vec3f&, const vec3f&);
	void ComputeAvgVelocity();
	void Reset();

	int GetGridWidth() const { return mWidth; }
	int GetGridHeight() const { return mHeight; }

	const float* GetHeightDataArray() const { return mHeightData; }

private:
	int mWidth;
	int mHeight;
	int mSquareSize;
	int mDownScale;

	float* mHeightData;
	float* mDensityData;

	ICallOutHandler *mCoh;

	std::map<unsigned int, Cell*> mTouchedCells;
	std::vector<Cell*> mCells;
	std::vector<Face*> mFaces;

	Face* CreateFace();
	vec3i Real2Grid(const vec3f&);
};

struct Face {
	float dPotential;
	vec3f velocity;
};

struct Cell {
	Cell(unsigned int _x, unsigned int _y):
		x(_x),
		y(_y)
	{}

	void ResetFull() {
		ResetDynamicVars();
		height = 0.0f;
		for (int dir = 0; dir < DIRECTIONS; dir++)
			dHeight[dir] = 0.0f;
	}

	void ResetDynamicVars() {
		ResetGroupVars();
		avgVelocity = NVECf;
		density     = 0.0f;
		discomfort = 0.0f;
	}

	void ResetGroupVars() {
		potential  = std::numeric_limits<float>::max();
		for (int dir = 0; dir < DIRECTIONS; dir++) {
			speed[dir] = 0.0f;
			cost[dir]  = 0.0f;
			faces[dir]->dPotential = 0.0f;
			faces[dir]->velocity = NVECf;
		}
	}


	unsigned int x, y;
	float discomfort;
	float potential;
	float density;
	float height;
	float dHeight[DIRECTIONS];
	float speed[DIRECTIONS];
	float cost[DIRECTIONS];
	Face* faces[DIRECTIONS];
	vec3f avgVelocity;
};

#endif
