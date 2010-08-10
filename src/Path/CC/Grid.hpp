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

		// for less() (NOTE: candidates are sorted in increasing order)
		bool operator() (const Cell* a, const Cell* b) const {
			return (a->potential > b->potential);
		}

		void ResetFull();
		void ResetDynamicVars();
		void ResetGroupVars();

		vec3f GetNormalizedPotentialGradient(unsigned int dir) const { return (edges[dir]->gradPotential / edges[dir]->gradPotential.len3D()); }
		vec3f GetInterpolatedVelocity(const vec3f&) const;

		struct Edge {
			vec3f gradPotential;
			vec3f velocity;
			vec3f gradHeight;
		};

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
	void UpdateSimObjectLocation(const unsigned int);
	void Reset();

	int GetGridWidth() const { return mWidth; }
	int GetGridHeight() const { return mHeight; }

	const float* GetHeightVisDataArray() const { return (mHeightVisData.empty())? NULL: &mHeightVisData[0]; }
	const float* GetSpeedVisDataArray() const { return (mSpeedVisData.empty())? NULL: &mSpeedVisData[0]; }
	const float* GetCostVisDataArray() const { return (mCostVisData.empty())? NULL: &mCostVisData[0]; }
	const float* GetPotentialVisDataArray() const { return (mPotentialVisData.empty())? NULL: &mPotentialVisData[0]; }
	const float* GetDensityVisDataArray() const { return (mDensityVisData.empty())? NULL: &mDensityVisData[0]; }
	const vec3f* GetVelocityAvgVisDataArray() const { return (mAvgVelocityVisData.empty())? NULL: &mAvgVelocityVisData[0]; }

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
	std::vector<float> mHeightVisData;
	std::vector<float> mSpeedVisData;
	std::vector<float> mCostVisData;
	std::vector<float> mPotentialVisData;
	std::vector<float> mDensityVisData;
	std::vector<vec3f> mAvgVelocityVisData;

	ICallOutHandler* mCOH;

	std::map<unsigned int, Cell*> mTouchedCells;
	std::vector<Cell> mCells;
	std::vector<Cell> mCellsBackup;
	std::vector<Cell::Edge> mEdges;
	std::vector<Cell::Edge> mEdgesBackup;

	Cell::Edge* CreateEdge();

	vec3i World2Grid(const vec3f&) const;
	vec3f Grid2World(const Cell*) const;
	void UpdateCandidates(const Cell*);

	void ComputeSpeedAndUnitCost(Cell*);
	float Potential2D(const float, const float, const float, const float) const;
	float Potential1D(const float, const float) const;
};

#endif
