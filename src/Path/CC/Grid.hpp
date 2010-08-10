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

		vec3f GetNormalizedPotentialGradient(unsigned int dir) const { return (edges[dir]->gradPotential / edges[dir]->gradPotential.len2D()); }
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
	void UpdateGroupPotentialField(unsigned int, const std::vector<Cell*>&, const std::set<unsigned int>&);
	void UpdateSimObjectLocation(const unsigned int);
	void Reset();

	int GetGridWidth() const { return mWidth; }
	int GetGridHeight() const { return mHeight; }

	// visualisation data accessors for scalar fields
	const float* GetDensityVisDataArray() const { return (mDensityVisData.empty())? NULL: &mDensityVisData[0]; }
	const float* GetDiscomfortVisDataArray() const { return (mDiscomfortVisData.empty())? NULL: &mDiscomfortVisData[0]; }
	const float* GetSpeedVisDataArray() const { return (mSpeedVisData.empty())? NULL: &mSpeedVisData[0]; }
	const float* GetCostVisDataArray() const { return (mCostVisData.empty())? NULL: &mCostVisData[0]; }
	const float* GetHeightVisDataArray() const { return (mHeightVisData.empty())? NULL: &mHeightVisData[0]; }
	const float* GetPotentialVisDataArray() const { return (mPotentialVisData.empty())? NULL: &mPotentialVisData[0]; }

	// visualisation data accessors for vector fields
	const vec3f* GetVelocityVisDataArray() const { return (mVelocityVisData.empty())? NULL: &mVelocityVisData[0]; }
	const vec3f* GetVelocityAvgVisDataArray() const { return (mAvgVelocityVisData.empty())? NULL: &mAvgVelocityVisData[0]; }
	const vec3f* GetPotentialDeltaVisDataArray() const { return (mPotentialDeltaVisData.empty())? NULL: &mPotentialDeltaVisData[0]; }
	const vec3f* GetHeightDeltaVisDataArray() const { return (mHeightDeltaVisData.empty())? NULL: &mHeightDeltaVisData[0]; }

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
	std::vector<float> mDiscomfortVisData;     // TODO: fill me
	std::vector<float> mSpeedVisData;          // TODO: fill me
	std::vector<float> mCostVisData;           // TODO: fill me
	std::vector<float> mPotentialVisData;
	std::vector<float> mHeightVisData;

	// visualization data for vector fields
	std::vector<vec3f> mVelocityVisData;
	std::vector<vec3f> mAvgVelocityVisData;
	std::vector<vec3f> mPotentialDeltaVisData; // TODO: fill me
	std::vector<vec3f> mHeightDeltaVisData;    // TODO: fill me

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
