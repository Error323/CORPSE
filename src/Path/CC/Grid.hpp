#ifndef PFFG_GRID_HDR
#define PFFG_GRID_HDR

#include <vector>
#include <map>
#include <set>
#include <queue>
#include <list>

#include "../../Math/vec3fwd.hpp"
#include "../../Math/vec3.hpp"

class ICallOutHandler;
class Grid {
public:
	enum {
		// NOTE: This order should not be changed!
		DIRECTION_NORTH = 0,
		DIRECTION_EAST  = 1,
		DIRECTION_SOUTH = 2,
		DIRECTION_WEST  = 3,
		NUM_DIRECTIONS  = 4
	};

	struct Cell {
		struct Edge {
			vec3f velocity;
			vec3f gradPotential;
			vec3f gradHeight;
		};

		Cell(): x(0), y(0), known(false), candidate(false), numNeighbors(0) {
			edges[DIRECTION_NORTH] = 0; neighbors[DIRECTION_NORTH] = 0;
			edges[DIRECTION_EAST ] = 0; neighbors[DIRECTION_EAST ] = 0;
			edges[DIRECTION_SOUTH] = 0; neighbors[DIRECTION_SOUTH] = 0;
			edges[DIRECTION_WEST ] = 0; neighbors[DIRECTION_WEST ] = 0;
		}

		Cell(unsigned int _x, unsigned int _y): x(_x), y(_y), known(false), candidate(false), numNeighbors(0) {
			edges[DIRECTION_NORTH] = 0; neighbors[DIRECTION_NORTH] = 0;
			edges[DIRECTION_EAST ] = 0; neighbors[DIRECTION_EAST ] = 0;
			edges[DIRECTION_SOUTH] = 0; neighbors[DIRECTION_SOUTH] = 0;
			edges[DIRECTION_WEST ] = 0; neighbors[DIRECTION_WEST ] = 0;
		}

		// for less() (NOTE: candidates are sorted in increasing order)
		bool operator() (const Cell* a, const Cell* b) const {
			return (a->potential > b->potential);
		}

		void ResetFull();
		void ResetGlobalStaticVars();
		void ResetGlobalDynamicVars();
		void ResetGroupVars();

		vec3f GetNormalizedPotentialGradient(const std::vector<Edge>&, unsigned int) const;
		vec3f GetInterpolatedVelocity(const std::vector<Edge>&, const vec3f&) const;

		unsigned int x, y;
		bool  known;
		bool  candidate;
		float discomfort;
		float potential;
		float density;
		float height;
		float speed[NUM_DIRECTIONS];
		float cost[NUM_DIRECTIONS];
		vec3f avgVelocity;

		unsigned int edges[NUM_DIRECTIONS];
		unsigned int neighbors[NUM_DIRECTIONS];
		unsigned int numNeighbors;
	};

	Grid(): mWidth(0), mHeight(0), mSquareSize(0), mDownScale(0), numResets(0) {
		mFrontBufferIdx = 0;
		mBackBufferIdx = 1;
	}

	void Init(const int, ICallOutHandler*);
	void Kill(const std::map<unsigned int, std::set<unsigned int> >&);
	void AddDensityAndVelocity(const vec3f&, const vec3f&);
	void ComputeAvgVelocity();
	void UpdateGroupPotentialField(unsigned int, const std::vector<unsigned int>&, const std::set<unsigned int>&);
	void UpdateSimObjectLocation(const unsigned int);
	void Reset();

	unsigned int GetGridWidth() const { return mWidth; }
	unsigned int GetGridHeight() const { return mHeight; }

	void AddGroup(unsigned int);
	void DelGroup(unsigned int);

	// visualisation data accessors for scalar fields
	const float* GetDensityVisDataArray() const;
	const float* GetHeightVisDataArray() const;
	const float* GetDiscomfortVisDataArray(unsigned int) const;
	const float* GetSpeedVisDataArray(unsigned int) const;
	const float* GetCostVisDataArray(unsigned int) const;
	const float* GetPotentialVisDataArray(unsigned int) const;

	// visualisation data accessors for vector fields
	const vec3f* GetHeightDeltaVisDataArray() const;
	const vec3f* GetVelocityAvgVisDataArray() const;
	const vec3f* GetVelocityVisDataArray(unsigned int) const;
	const vec3f* GetPotentialDeltaVisDataArray(unsigned int) const;

	unsigned int World2Cell(const vec3f&) const;

private:
	static const float sLambda;
	static const float sMinDensity;

	unsigned int mWidth;
	unsigned int mHeight;
	unsigned int mSquareSize;
	unsigned int mDownScale;
	unsigned int numResets;

	float mMinSlope;
	float mMaxSlope;
	float mMaxSpeed;
	float mMaxRadius;

	// FMM vars
	std::priority_queue<Cell*, std::vector<Cell*, std::allocator<Cell*> >, Cell> mCandidates;

	// visualization data for scalar fields
	std::vector<float> mDensityVisData;
	std::vector<float> mHeightVisData;
	std::map<unsigned int, std::vector<float> > mDiscomfortVisData;
	std::map<unsigned int, std::vector<float> > mSpeedVisData;
	std::map<unsigned int, std::vector<float> > mCostVisData;
	std::map<unsigned int, std::vector<float> > mPotentialVisData;

	// visualization data for vector fields
	std::vector<vec3f> mHeightDeltaVisData;
	std::vector<vec3f> mAvgVelocityVisData;
	std::map<unsigned int, std::vector<vec3f> > mVelocityVisData;
	std::map<unsigned int, std::vector<vec3f> > mPotentialDeltaVisData;

	ICallOutHandler* mCOH;

	// cells that were modified by the AddDensityAndVelocity step
	// (which sets the Cell::avgVelocity and Cell::density globals)
	std::set<unsigned int> mTouchedCells;


	// these contain the grid-state after initializing only
	// the *static* global data; their contents are used to
	// clear both buffers at the start of each frame
	std::vector<Cell      > mInitCells;
	std::vector<Cell::Edge> mInitEdges;

	// these contain the grid-state after also initializing
	// the dynamic global data; the buffers are cycled after
	// processing a group
	struct Buffer {
		std::vector<Cell      > cells;
		std::vector<Cell::Edge> edges;
	};
	Buffer mBuffers[2];

	unsigned int mFrontBufferIdx;
	unsigned int mBackBufferIdx;



	vec3i World2Grid(const vec3f&) const;
	vec3f Grid2World(const Cell*) const;
	void UpdateCandidates(unsigned int, const Cell*);

	void ComputeSpeedAndUnitCost(unsigned int, Cell*);
	float Potential2D(const float, const float, const float, const float) const;
	float Potential1D(const float, const float) const;
};

#endif
