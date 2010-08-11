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
		Cell(): x(0), y(0), known(false), candidate(false), numNeighbours(0) {
			edges[DIRECTION_NORTH] = NULL; neighbours[DIRECTION_NORTH] = NULL;
			edges[DIRECTION_EAST ] = NULL; neighbours[DIRECTION_EAST ] = NULL;
			edges[DIRECTION_SOUTH] = NULL; neighbours[DIRECTION_SOUTH] = NULL;
			edges[DIRECTION_WEST ] = NULL; neighbours[DIRECTION_WEST ] = NULL;
		}

		Cell(unsigned int _x, unsigned int _y): x(_x), y(_y), known(false), candidate(false), numNeighbours(0) {
			edges[DIRECTION_NORTH] = NULL; neighbours[DIRECTION_NORTH] = NULL;
			edges[DIRECTION_EAST ] = NULL; neighbours[DIRECTION_EAST ] = NULL;
			edges[DIRECTION_SOUTH] = NULL; neighbours[DIRECTION_SOUTH] = NULL;
			edges[DIRECTION_WEST ] = NULL; neighbours[DIRECTION_WEST ] = NULL;
		}

		// for less() (NOTE: candidates are sorted in increasing order)
		bool operator() (const Cell* a, const Cell* b) const {
			return (a->potential > b->potential);
		}

		void ResetFull();
		void ResetGlobalStaticVars();
		void ResetGlobalDynamicVars();
		void ResetGroupVars();

		vec3f GetNormalizedPotentialGradient(unsigned int) const;
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
	~Grid() {}

	void Init(const int, ICallOutHandler*);
	void Kill(const std::map<unsigned int, std::set<unsigned int> >&);
	void AddDensityAndVelocity(const vec3f&, const vec3f&);
	void ComputeAvgVelocity();
	void UpdateGroupPotentialField(unsigned int, const std::vector<Cell*>&, const std::set<unsigned int>&);
	void UpdateSimObjectLocation(const unsigned int);
	void Reset();

	int GetGridWidth() const { return mWidth; }
	int GetGridHeight() const { return mHeight; }

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

	std::map<unsigned int, Cell*> mTouchedCells;
	std::vector<Cell> mCells;
	std::vector<Cell> mCellsBackup;
	std::vector<Cell::Edge> mEdges;
	std::vector<Cell::Edge> mEdgesBackup;

	Cell::Edge* CreateEdge();

	vec3i World2Grid(const vec3f&) const;
	vec3f Grid2World(const Cell*) const;
	void UpdateCandidates(unsigned int, const Cell*);

	void ComputeSpeedAndUnitCost(unsigned int, Cell*);
	float Potential2D(const float, const float, const float, const float) const;
	float Potential1D(const float, const float) const;
};

#endif
