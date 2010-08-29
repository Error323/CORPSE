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
		DIR_N    = 0,
		DIR_S    = 1,
		DIR_E    = 2,
		DIR_W    = 3,
		NUM_DIRS = 4
	};

	enum {
		// scalar fields
		DATATYPE_DENSITY         = 0, // rho (global,    stored at cell-centers, 1 scalar  per cell)
		DATATYPE_HEIGHT          = 1, // h   (global,    stored at cell-centers, 1 scalar  per cell)
		DATATYPE_DISCOMFORT      = 2, // g   (global,    stored at cell-centers, 1 scalar  per cell)
		DATATYPE_SPEED           = 3, // f   (per-group, stored at cell-edges,   4 scalars per cell)
		DATATYPE_COST            = 4, // C   (per-group, stored at cell-edges,   4 scalars per cell)
		DATATYPE_POTENTIAL       = 5, // phi (per-group, stored at cell-centers, 1 scalar  per cell)

		NUM_SCALAR_DATATYPES     = 6,
	};
	enum {
		// vector fields
		DATATYPE_HEIGHT_DELTA    = 6, // delta-h   (global,    stored at cell-edges,   4 vectors per cell)
		DATATYPE_VELOCITY_AVG    = 7, // v-bar     (global,    stored at cell-centers, 1 vector  per cell)
		DATATYPE_VELOCITY        = 8, // v         (per-group, stored at cell-edges,   4 vectors per cell)
		DATATYPE_POTENTIAL_DELTA = 9, // delta-phi (per-group, stored at cell-edges,   4 vectors per cell)

		NUM_VECTOR_DATATYPES     = 4,
	};

	struct Cell {
		struct Edge {
			vec3f velocity;
			vec3f heightDelta;
			vec3f potentialDelta;
		};

		Cell(): x(0), y(0), known(false), candidate(false), numNeighbors(0) {
			edges[DIR_N] = 0; neighbors[DIR_N] = 0;
			edges[DIR_E] = 0; neighbors[DIR_E] = 0;
			edges[DIR_S] = 0; neighbors[DIR_S] = 0;
			edges[DIR_W] = 0; neighbors[DIR_W] = 0;
		}

		Cell(unsigned int _x, unsigned int _y): x(_x), y(_y), known(false), candidate(false), numNeighbors(0) {
			edges[DIR_N] = 0; neighbors[DIR_N] = 0;
			edges[DIR_E] = 0; neighbors[DIR_E] = 0;
			edges[DIR_S] = 0; neighbors[DIR_S] = 0;
			edges[DIR_W] = 0; neighbors[DIR_W] = 0;
		}

		// for less() (NOTE: candidates are sorted in increasing order)
		bool operator() (const Cell* a, const Cell* b) const {
			return (a->potential > b->potential);
		}

		void ResetFull();
		void ResetGlobalStaticVars();
		void ResetGlobalDynamicVars();
		void ResetGroupVars();

		vec3f GetNormalisedPotentialGradient(const std::vector<Edge>&, unsigned int) const;
		vec3f avgVelocity;

		unsigned int x, y;
		bool  known;
		bool  candidate;
		float discomfort;
		float potential;
		float density;
		float height;

		// NOTE:
		//    redundant, f(A --> B) is the same as f(B --> A) but
		//    now stored twice (A.speed[DIR_N] and B.speed[DIR_S])
		float speed[NUM_DIRS];
		float cost[NUM_DIRS];

		unsigned int edges[NUM_DIRS];
		unsigned int neighbors[NUM_DIRS];
		unsigned int numNeighbors;
	};

	Grid(): numCellsX(0), numCellsZ(0), mSquareSize(0), mDownScale(0) {
		mDirVectors[DIR_N] = -ZVECf;
		mDirVectors[DIR_S] =  ZVECf;
		mDirVectors[DIR_E] =  XVECf;
		mDirVectors[DIR_W] = -XVECf;
 
		mCurrBufferIdx = 0;
		mPrevBufferIdx = 1;
	}

	void Init(unsigned int, ICallOutHandler*);
	void Kill();
	void Reset();

	void AddGlobalDynamicCellData(std::vector<Cell>&, std::vector<Cell>&, const Cell*, int, const vec3f&, unsigned int);
	void AddDensity(const vec3f&, const vec3f&, float);
	void AddDiscomfort(const vec3f&, const vec3f&, float, unsigned int, float);
	void ComputeAvgVelocity();

	void UpdateGroupPotentialField(unsigned int, const std::set<unsigned int>&, const std::set<unsigned int>&);
	bool UpdateSimObjectLocation(unsigned int, unsigned int);

	void AddGroup(unsigned int);
	void DelGroup(unsigned int);

	// visualisation data accessors for scalar fields
	const float* GetDensityVisDataArray() const;
	const float* GetHeightVisDataArray() const;
	const float* GetDiscomfortVisDataArray() const;
	const float* GetSpeedVisDataArray(unsigned int) const;
	const float* GetCostVisDataArray(unsigned int) const;
	const float* GetPotentialVisDataArray(unsigned int) const;

	// visualisation data accessors for vector fields
	const vec3f* GetHeightDeltaVisDataArray() const;
	const vec3f* GetVelocityAvgVisDataArray() const;
	const vec3f* GetVelocityVisDataArray(unsigned int) const;
	const vec3f* GetPotentialDeltaVisDataArray(unsigned int) const;

	unsigned int WorldPosToCellID(const vec3f&) const;
	const Cell* GetCell(unsigned int idx, unsigned int buf = 0) const { return &mBuffers[buf].cells[idx]; }
	vec3f GetCellPos(const Cell* c) const { return vec3f((c->x * mSquareSize) + (mSquareSize >> 1), 0.0f, (c->y * mSquareSize) + (mSquareSize >> 1)); }

	unsigned int GetGridWidth() const { return numCellsX; }
	unsigned int GetGridHeight() const { return numCellsZ; }
	unsigned int GetSquareSize() const { return mSquareSize; }

private:
	float mRhoMin;
	float mRhoMax;
	float mRhoBar;
	float mAlphaWeight;
	float mBetaWeight;
	float mGammaWeight;

	unsigned int numCellsX;
	unsigned int numCellsZ;
	unsigned int mSquareSize;
	unsigned int mDownScale;

	float mMinGroupSlope, mMinTerrainSlope; // ?, sMin (not normalised)
	float mMaxGroupSlope, mMaxTerrainSlope; // ?, sMax (not normalised)
	float mMinGroupSpeed;                   // fMin
	float mMaxGroupSpeed;                   // fMax
	float mMaxGroupRadius;

	// used by UpdateGroupPotentialField() to
	// validate potential-field construction
	unsigned int numInfinitePotentialCases;
	unsigned int numIllegalDirectionCases;

	// FMM vars
	std::priority_queue<Cell*, std::vector<Cell*, std::allocator<Cell*> >, Cell> mCandidates;

	// visualization data for scalar fields
	std::vector<float> mDensityVisData;
	std::vector<float> mHeightVisData;
	std::vector<float> mDiscomfortVisData;
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
	// (which sets the Cell::avgVelocity and Cell::density dynamic
	// globals)
	std::set<unsigned int> mTouchedCells;

	// these buffers are cycled after processing a group; during
	// UpdateGroupPotentialField, one buffer is made dirty (wrt.
	// the per-group dynamic data) while the other is cleaned
	struct Buffer {
		std::vector<Cell      > cells;
		std::vector<Cell::Edge> edges;
	};
	Buffer mBuffers[2];

	unsigned int mCurrBufferIdx;
	unsigned int mPrevBufferIdx;


	// world-space directions corresponding to NSEW
	vec3f mDirVectors[NUM_DIRS];

	vec3f GetInterpolatedVelocity(const std::vector<Cell::Edge>&, const Cell*, const vec3f&, const vec3f&) const;
	vec3i WorldPosToGridIdx(const vec3f&) const;
	vec3f GridIdxToWorldPos(const Cell*) const;

	void ComputeCellSpeed(unsigned int, unsigned int, std::vector<Cell>&, std::vector<Cell::Edge>&);
	void ComputeCellCost(unsigned int, unsigned int, std::vector<Cell>&, std::vector<Cell::Edge>&);
	void ComputeCellSpeedAndCost(unsigned int, unsigned int, std::vector<Cell>&, std::vector<Cell::Edge>&);
	void ComputeCellSpeedAndCostMERGED(unsigned int, Cell*, std::vector<Cell>&, std::vector<Cell::Edge>&);
	void ComputeSpeedAndCost(unsigned int);

	void UpdateCandidates(unsigned int, const Cell*);
	float Potential2D(const float, const float, const float, const float) const;
	float Potential1D(const float, const float) const;
};

#endif
