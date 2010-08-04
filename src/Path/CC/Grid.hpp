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

	void Init(const int, ICallOutHandler*);
	void AddDensityAndSpeed(const vec3f&, const vec3f&);
	void Reset();
	static vec3f Grid2Real(Cell*);
	Cell* operator[] (size_t i) { return mCells[i]; }

private:
	int mWidth;
	int mHeight;
	int mSquareSize;

	ICallOutHandler *mCoh;

	std::map<unsigned int, Cell*> mTouchedCells;
	std::vector<Cell*> mCells;
	std::vector<Face*> mFaces;

	Face* CreateFace();
	vec3f Real2Grid(const vec3f&);
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
