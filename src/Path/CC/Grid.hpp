#ifndef PFFG_GRID_HDR
#define PFFG_GRID_HDR

#include <vector>
#include <map>
#include <set>
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
	void ComputeSpeedFieldAndUnitCost(const std::set<unsigned int>&);
	void UpdateGroupPotentialField(const std::vector<Cell*>&);
	void UpdateSimObjectLocation(const int);
	void Reset();

	int GetGridWidth()                  const { return mWidth; }
	int GetGridHeight()                 const { return mHeight; }

	const float* GetHeightDataArray()   const { return &mHeightData[0]; }
	const float* GetDensityDataArray()  const { return &mDensityData[0]; }
	const vec3f* GetVelocityDataArray() const { return &mVelocityData[0]; }

private:
	int mWidth;
	int mHeight;
	int mSquareSize;
	int mDownScale;

	float mLambda;
	float mMinDensity;

	// Visualization data
	std::vector<float> mHeightData;
	std::vector<float> mDensityData;
	std::vector<vec3f> mVelocityData;

	ICallOutHandler *mCoh;

	std::map<unsigned int, Cell*> mTouchedCells;
	std::vector<Cell*> mCells;
	std::vector<Face*> mFaces;

	Face* CreateFace();
	vec3i Real2Grid(const vec3f&);
};

struct Face {
	vec3f gradPotential;
	vec3f velocity;
	vec3f gradHeight;
};

struct Cell {
	Cell(unsigned int _x, unsigned int _y):
		x(_x),
		y(_y)
	{}

	void ResetFull();
	void ResetDynamicVars();
	void ResetGroupVars();

	unsigned int x, y;
	float discomfort;
	float potential;
	float density;
	float height;
	float speed[DIRECTIONS];
	float cost[DIRECTIONS];
	Face* faces[DIRECTIONS];
	vec3f avgVelocity;
};

#endif
