#ifndef PFFG_GRID_HDR
#define PFFG_GRID_HDR

#include <vector>
#include <map>
#include <set>
#include <queue>
#include <list>

#include "../../Math/vec3fwd.hpp"
#include "../../Math/vec3.hpp"
#include "../../Ext/ICallOutHandler.hpp"

// NOTE: This order should not be changed!
enum {
	DIRECTION_NORTH = 0,
	DIRECTION_EAST  = 1,
	DIRECTION_SOUTH = 2,
	DIRECTION_WEST  = 3,
	NUM_DIRECTIONS  = 4
};



class Grid {
public:
	struct Cell {
		Cell(): x(0), y(0), known(false), candidate(false), numNeighbours(0) {
		}

		Cell(unsigned int _x, unsigned int _y): x(_x), y(_y), known(false), candidate(false), numNeighbours(0) {
		}

		struct Edge {
			vec3f gradPotential;
			vec3f velocity;
			vec3f gradHeight;
		};

		// for less() (NOTE: candidates are sorted in increasing order)
		bool operator() (const Cell* a, const Cell* b) const {
			return (a->potential > b->potential);
		}

		void ResetFull();
		void ResetDynamicVars();
		void ResetGroupVars();

		unsigned int x, y;
		bool  known;
		bool  candidate;
		float discomfort;
		float potential;
		float density;
		float height;
		float speed[NUM_DIRECTIONS];
		float cost[NUM_DIRECTIONS];
		Edge* edges[NUM_DIRECTIONS];
		vec3f avgVelocity;
		Cell* neighbours[NUM_DIRECTIONS];
		int   numNeighbours;
	};

	Grid(): mWidth(0), mHeight(0), mSquareSize(0), mDownScale(0), numResets(0) {}
	~Grid();

	void Init(const int, ICallOutHandler*);
	void AddDensityAndVelocity(const vec3f&, const vec3f&);
	void ComputeAvgVelocity();
	void UpdateGroupPotentialField(const std::vector<Cell*>&, const std::set<unsigned int>&);
	void UpdateSimObjectLocation(const int);
	void Reset();

	int GetGridWidth() const { return mWidth; }
	int GetGridHeight() const { return mHeight; }

	const float* GetHeightDataArray()    const { return (mHeightData.empty())? NULL: &mHeightData[0]; }
	const float* GetPotentialDataArray() const { return (mPotentialData.empty())? NULL: &mPotentialData[0]; }
	const float* GetDensityDataArray()   const { return (mDensityData.empty())? NULL: &mDensityData[0]; }
	const vec3f* GetVelocityDataArray()  const { return (mVelocityData.empty())? NULL: &mVelocityData[0]; }

	Cell* World2Cell(const vec3f&);

private:
	static const float sLambda;
	static const float sMinDensity;

	int mWidth;
	int mHeight;
	int mSquareSize;
	int mDownScale;
	int numResets;

	float mMinSlope;
	float mMaxSlope;
	float mMaxSpeed;
	float mMaxRadius;

	// FMM vars
	std::priority_queue<Cell*, std::vector<Cell*, std::allocator<Cell*> >, Cell> mCandidates;

	// Visualization data
	std::vector<float> mHeightData;
	std::vector<float> mPotentialData;
	std::vector<float> mDensityData;
	std::vector<vec3f> mVelocityData;

	ICallOutHandler* mCoh;

	std::map<unsigned int, Cell*> mTouchedCells;
	std::vector<Cell> mCells;
	std::vector<Cell> mCellsBackup;
	std::vector<Cell::Edge> mEdges;
	std::vector<Cell::Edge> mEdgesBackup;

	Cell::Edge* CreateEdge();

	vec3i World2Grid(const vec3f&);
	vec3f Grid2World(const Cell*);
	void UpdateCandidates(const Cell*);

	void ComputeSpeedAndUnitCost(Cell*);
	float Potential2D(const float, const float, const float, const float);
	float Potential1D(const float, const float);
};

#endif
