#include <cmath>
#include <algorithm>
#include <limits>

#include "./CCGrid.hpp"
#include "../../Ext/ICallOutHandler.hpp"
#include "../../Math/Trig.hpp"
#include "../../Sim/SimObjectDef.hpp"
#include "../../Sim/SimObjectState.hpp"
#include "../../System/Debugger.hpp"

#define EPSILON 0.01f

#define GRID_INDEX_UNSAFE(x, y) (((y) * (numCellsX)) + (x))
#define GRID_INDEX_CLAMPED(x, y) (((CLAMP((y), 0, (numCellsZ - 1))) * (numCellsX)) + (CLAMP((x), 0, (numCellsX - 1))))

#define ELEVATION(x, y) (mCOH->GetCenterHeightMap()[(mDownScale * (y)) * (mDownScale * numCellsX) + (mDownScale * (x))])
// given the grid resolution, find the discrete number of cells
// <n> spanned by <r> (the *radius* of the disc projected by an
// agent onto the grid; used when TCP06-style density conversion
// is DISabled)
// note: the total diameter of the projected disc (in cells) is
// always <n + 1 + n>; see AddGlobalDynamicCellData() (therefore
// the first unaffected cell along NSEW is located at (x, y) +/-
// (n + 1, n + 1) where (x, y) is the disc's center-cell)
#define CELLS_IN_RADIUS(r) ((r / (mSquareSize >> 1)) + 1)

#define POSITIVE_SLOPE(dir, slope)                            \
	(((dir == DIR_N || dir == DIR_W) && (slope <  0.0f))  ||  \
	 ((dir == DIR_S || dir == DIR_E) && (slope >= 0.0f)))
#define NEGATIVE_SLOPE(dir, slope)                            \
	(((dir == DIR_N || dir == DIR_W) && (slope >= 0.0f))  ||  \
	 ((dir == DIR_S || dir == DIR_E) && (slope <  0.0f)))

#define MMAX(a, b) (((a) >= (b))? (a): (b))
#define MMIN(a, b) (((a) >= (b))? (b): (a))
#define MMIX(a, b, t) ((a) * t + (b) * (1.0f - t))
#define CLAMP(v, vmin, vmax) MMAX((vmin), MMIN((vmax), (v)))

#define SPEED_COST_SHARED_NEIGHBOR_CELL         1
#define SPEED_COST_POTENTIAL_MERGED_COMPUTATION 1
#define SPEED_COST_SINGLE_PASS_COMPUTATION      0
#define SPEED_COST_DIRECTIONAL_DISCOMFORT       1

#define VELOCITY_FIELD_DIRECT_INTERPOLATION     0
#define VELOCITY_FIELD_BILINEAR_INTERPOLATION   1



void CCGrid::AddGroup(unsigned int groupID) {
	mSpeedVisData[groupID] = std::vector<float>();
	mSpeedVisData[groupID].resize(numCellsX * numCellsZ * NUM_DIRS, 0.0f);
	mCostVisData[groupID] = std::vector<float>();
	mCostVisData[groupID].resize(numCellsX * numCellsZ * NUM_DIRS, 0.0f);

	mPotentialVisData[groupID] = std::vector<float>();
	mPotentialVisData[groupID].resize(numCellsX * numCellsZ, 0.0f);

	mVelocityVisData[groupID] = std::vector<vec3f>();
	mVelocityVisData[groupID].resize(numCellsX * numCellsZ * NUM_DIRS, NVECf);

	mPotentialDeltaVisData[groupID] = std::vector<vec3f>();
	mPotentialDeltaVisData[groupID].resize(numCellsX * numCellsZ * NUM_DIRS, NVECf);

	mGroupGridStates[groupID] = Buffer(numCellsX, numCellsZ);
}

void CCGrid::DelGroup(unsigned int groupID) {
	mSpeedVisData[groupID].clear(); mSpeedVisData.erase(groupID);
	mCostVisData[groupID].clear(); mCostVisData.erase(groupID);
	mPotentialVisData[groupID].clear(); mPotentialVisData.erase(groupID);

	mVelocityVisData[groupID].clear(); mVelocityVisData.erase(groupID);
	mPotentialDeltaVisData[groupID].clear(); mPotentialDeltaVisData.erase(groupID);

	mGroupGridStates.erase(groupID);
}



// visualisation data accessors for scalar fields
const float* CCGrid::GetDensityVisDataArray() const {
	return (mDensityVisData.empty())? NULL: &mDensityVisData[0];
}

const float* CCGrid::GetSpeedVisDataArray(unsigned int groupID) const {
	std::map<unsigned int, std::vector<float> >::const_iterator it = mSpeedVisData.find(groupID);

	if (it == mSpeedVisData.end()) { return NULL; }
	if ((it->second).empty()) { return NULL; }

	return &(it->second)[0];
}

const float* CCGrid::GetCostVisDataArray(unsigned int groupID) const {
	std::map<unsigned int, std::vector<float> >::const_iterator it = mCostVisData.find(groupID);

	if (it == mCostVisData.end()) { return NULL; }
	if ((it->second).empty()) { return NULL; }

	return &(it->second)[0];
}

const float* CCGrid::GetHeightVisDataArray() const {
	return (mHeightVisData.empty())? NULL: &mHeightVisData[0];
}

const float* CCGrid::GetPotentialVisDataArray(unsigned int groupID) const {
	std::map<unsigned int, std::vector<float> >::const_iterator it = mPotentialVisData.find(groupID);

	if (it == mPotentialVisData.end()) { return NULL; }
	if ((it->second).empty()) { return NULL; }

	return &(it->second)[0];
}

// visualisation data accessors for vector fields
const vec3f* CCGrid::GetDiscomfortVisDataArray() const {
	return (mDiscomfortVisData.empty())? NULL: &mDiscomfortVisData[0];
}

const vec3f* CCGrid::GetVelocityVisDataArray(unsigned int groupID) const {
	std::map<unsigned int, std::vector<vec3f> >::const_iterator it = mVelocityVisData.find(groupID);

	if (it == mVelocityVisData.end()) { return NULL; }
	if ((it->second).empty()) { return NULL; }

	return &(it->second)[0];
}

const vec3f* CCGrid::GetVelocityAvgVisDataArray() const {
	return (mAvgVelocityVisData.empty())? NULL: &mAvgVelocityVisData[0];
}

const vec3f* CCGrid::GetPotentialDeltaVisDataArray(unsigned int groupID) const {
	std::map<unsigned int, std::vector<vec3f> >::const_iterator it = mPotentialDeltaVisData.find(groupID);

	if (it == mPotentialDeltaVisData.end()) { return NULL; }
	if ((it->second).empty()) { return NULL; }

	return &(it->second)[0];
}

const vec3f* CCGrid::GetHeightDeltaVisDataArray() const {
	return (mHeightDeltaVisData.empty())? NULL: &mHeightDeltaVisData[0];
}



void CCGrid::Kill() {
	// clear the global scalar field visualisation data
	mDensityVisData.clear();
	mHeightVisData.clear();

	// clear the global vector field visualisation data
	mAvgVelocityVisData.clear();
	mHeightDeltaVisData.clear();

	mTouchedCells.clear();

	mGridStates[mCurrBufferIdx].cells.clear(); mGridStates[mCurrBufferIdx].edges.clear();
	mGridStates[mPrevBufferIdx].cells.clear(); mGridStates[mPrevBufferIdx].edges.clear();
}

void CCGrid::Init(unsigned int downScaleFactor, ICallOutHandler* coh) {
	PFFG_ASSERT(downScaleFactor >= 1);

	static const char* tableNames[] = {
		"pathmodule", "cc", NULL,
	};

	mCOH        = coh;
	mDownScale  = downScaleFactor;
	numCellsX   = mCOH->GetHeightMapSizeX() / mDownScale;
	numCellsZ   = mCOH->GetHeightMapSizeZ() / mDownScale;
	mSquareSize = mCOH->GetSquareSize()     * mDownScale;

	mAlphaWeight = mCOH->GetFloatConfigParam(tableNames, "alpha",      -1.0f);
	mBetaWeight  = mCOH->GetFloatConfigParam(tableNames, "beta",       -1.0f);
	mGammaWeight = mCOH->GetFloatConfigParam(tableNames, "gamma",      -1.0f);
	mRhoBar      = mCOH->GetFloatConfigParam(tableNames, "rho_bar",    -1.0f);
	mRhoMin      = mCOH->GetFloatConfigParam(tableNames, "rho_min",    -1.0f);
	mRhoMax      = mCOH->GetFloatConfigParam(tableNames, "rho_max",    -1.0f);
	mUpdateInt   = mCOH->GetFloatConfigParam(tableNames, "updateInt",   1.0f);
	mUpdateMode  = mCOH->GetFloatConfigParam(tableNames, "updateMode",  1.0f);

	// NOTE:
	//   the slope (height difference) from A to B is equal to the inverse
	//   slope from B to A, therefore we take the absolute value at every
	//   cell (this means the scale term in f_topo lies in [-1, 1] rather
	//   than in [0, 1]) to determine the extrema
	// NOTE: height-map must be downsampled if mDownScale > 1
	mMinTerrainSlope =  std::numeric_limits<float>::max();
	mMaxTerrainSlope = -std::numeric_limits<float>::max();
	mFlatTerrain     = ((mCOH->GetMaxMapHeight() - mCOH->GetMinMapHeight()) < EPSILON);

	printf("[CCGrid::Init] resolution: %dx%d %d\n", numCellsX, numCellsZ, mSquareSize);
	printf("\tSPEED_COST_SHARED_NEIGHBOR_CELL:         %d\n", SPEED_COST_SHARED_NEIGHBOR_CELL);
	printf("\tSPEED_COST_POTENTIAL_MERGED_COMPUTATION: %d\n", SPEED_COST_POTENTIAL_MERGED_COMPUTATION);
	printf("\tSPEED_COST_SINGLE_PASS_COMPUTATION:      %d\n", SPEED_COST_SINGLE_PASS_COMPUTATION);
	printf("\tSPEED_COST_DIRECTIONAL_DISCOMFORT:       %d\n", SPEED_COST_DIRECTIONAL_DISCOMFORT);
	printf("\n");
	printf("\tVELOCITY_FIELD_DIRECT_INTERPOLATION:     %d\n", VELOCITY_FIELD_DIRECT_INTERPOLATION);
	printf("\tVELOCITY_FIELD_BILINEAR_INTERPOLATION:   %d\n", VELOCITY_FIELD_BILINEAR_INTERPOLATION);

	const unsigned int numCells = numCellsX * numCellsZ;
	const unsigned int numEdges = (numCellsX + 1) * numCellsZ + (numCellsZ + 1) * numCellsX;

	// visualisation data for global scalar fields
	mDensityVisData.resize(numCells, 0.0f);
	mHeightVisData.resize(numCells, 0.0f);

	// visualisation data for global vector fields
	mDiscomfortVisData.resize(numCells, NVECf);
	mAvgVelocityVisData.resize(numCells, NVECf);
	mHeightDeltaVisData.resize(numCells * NUM_DIRS);


	Buffer& currGridBuffer = mGridStates[mCurrBufferIdx];
	Buffer& prevGridBuffer = mGridStates[mPrevBufferIdx];

	std::vector<Cell      >& currCells = currGridBuffer.cells; currCells.reserve(numCells);
	std::vector<Cell      >& prevCells = prevGridBuffer.cells; prevCells.reserve(numCells);
	std::vector<Cell::Edge>& currEdges = currGridBuffer.edges; currEdges.reserve(numEdges);
	std::vector<Cell::Edge>& prevEdges = prevGridBuffer.edges; prevEdges.reserve(numEdges);

	for (unsigned int y = 0; y < numCellsZ; y++) {
		for (unsigned int x = 0; x < numCellsX; x++) {
			currCells.push_back(Cell(x, y));
			prevCells.push_back(Cell(x, y));
			currEdges.push_back(CCGrid::Cell::Edge()); // Wf
			currEdges.push_back(CCGrid::Cell::Edge()); // Nf
			prevEdges.push_back(CCGrid::Cell::Edge()); // Wb
			prevEdges.push_back(CCGrid::Cell::Edge()); // Nb

			Cell* currCell = &currCells.back();
			Cell* prevCell = &prevCells.back();

			unsigned int cellIdx = 0;
			unsigned int edgeIdxW = currEdges.size() - 2;
			unsigned int edgeIdxN = currEdges.size() - 1;

			currCell->edges[DIR_N] = edgeIdxN;
			currCell->edges[DIR_W] = edgeIdxW;
			prevCell->edges[DIR_N] = edgeIdxN;
			prevCell->edges[DIR_W] = edgeIdxW;

			// bind the east face of the cell west of the current cell
			if (x > 0) {
				cellIdx = GRID_INDEX_UNSAFE(x - 1, y);
				PFFG_ASSERT(cellIdx < currCells.size());

				Cell* currWestCell = &currCells[cellIdx];
				Cell* prevWestCell = &prevCells[cellIdx];
				currWestCell->edges[DIR_E] = edgeIdxW;
				prevWestCell->edges[DIR_E] = edgeIdxW;
			}

			// bind the south face of the cell north of the current cell
			if (y > 0) {
				cellIdx = GRID_INDEX_UNSAFE(x, y - 1);
				PFFG_ASSERT(cellIdx < currCells.size());

				Cell* currNorthCell = &currCells[cellIdx];
				Cell* prevNorthCell = &prevCells[cellIdx];
				currNorthCell->edges[DIR_S] = edgeIdxN;
				prevNorthCell->edges[DIR_S] = edgeIdxN;
			}

			// bind a new face to the southern face of the border cell
			if (y == numCellsZ - 1) {
				currEdges.push_back(CCGrid::Cell::Edge());
				prevEdges.push_back(CCGrid::Cell::Edge());
				currCell->edges[DIR_S] = currEdges.size() - 1;
				prevCell->edges[DIR_S] = prevEdges.size() - 1;
			}

			// bind a new face to the eastern face of the border cell
			if (x == numCellsX - 1) {
				currEdges.push_back(CCGrid::Cell::Edge());
				prevEdges.push_back(CCGrid::Cell::Edge());
				currCell->edges[DIR_E] = currEdges.size() - 1;
				prevCell->edges[DIR_E] = prevEdges.size() - 1;
			}
		}
	}

	PFFG_ASSERT(currCells.size() == numCells);
	PFFG_ASSERT(prevEdges.size() == numEdges);

	// perform a full reset of the cells and compute their heights
	for (unsigned int y = 0; y < numCellsZ; y++) {
		for (unsigned int x = 0; x < numCellsX; x++) {
			Cell* currCell = &currCells[GRID_INDEX_UNSAFE(x, y)];
			Cell* prevCell = &prevCells[GRID_INDEX_UNSAFE(x, y)];

			// set potential to +inf, etc.
			currCell->ResetFull();
			prevCell->ResetFull();

			// set the height, assuming the heightmap is static
			currCell->height = ELEVATION(x, y);
			prevCell->height = ELEVATION(x, y);

			// set the baseline static discomfort values
			// NOTE:
			//    this assumes all groups experience discomfort in the same way,
			//    because the field is global (converting it back to per-group
			//    again would make the predictive discomfort step too expensive)
			//    therefore, maximum-slope restrictions can not be modelled via
			//    discomfort zones
			//
			//    for now, avoid higher areas (problem: discomfort is a much larger
			//    term than speed, but needs to be around same order of magnitude)
			currCell->staticDiscomfort.y = mFlatTerrain? 0.0f: ((currCell->height - mCOH->GetMinMapHeight()) / (mCOH->GetMaxMapHeight() - mCOH->GetMinMapHeight()));
			prevCell->staticDiscomfort.y = mFlatTerrain? 0.0f: ((currCell->height - mCOH->GetMinMapHeight()) / (mCOH->GetMaxMapHeight() - mCOH->GetMinMapHeight()));

			mHeightVisData[GRID_INDEX_UNSAFE(x, y)] = currCell->height;
			mDiscomfortVisData[GRID_INDEX_UNSAFE(x, y)] = currCell->staticDiscomfort;
		}
	}

	// compute gradient-heights and neighbors
	for (unsigned int y = 0; y < numCellsZ; y++) {
		for (unsigned int x = 0; x < numCellsX; x++) {
			unsigned int idx = GRID_INDEX_UNSAFE(x, y);
			unsigned int dir = 0;

			Cell* currCell = &currCells[idx];
			Cell* prevCell = &prevCells[idx];

			Cell* currNgb = NULL;
			Cell* prevNgb = NULL;
			Cell::Edge* currEdge = NULL;
			Cell::Edge* prevEdge = NULL;

			if (y > 0) {
				dir = DIR_N;

				currEdge = &currEdges[currCell->edges[dir]];
				prevEdge = &prevEdges[prevCell->edges[dir]];
				currNgb = &currCells[GRID_INDEX_UNSAFE(x, y - 1)];
				prevNgb = &prevCells[GRID_INDEX_UNSAFE(x, y - 1)];

				currEdge->heightDelta = vec3f(0.0f, 0.0f, (currNgb->height - currCell->height));
				prevEdge->heightDelta = vec3f(0.0f, 0.0f, (prevNgb->height - prevCell->height));

				currCell->neighbors[currCell->numNeighbors++] = GRID_INDEX_UNSAFE(x, y - 1);
				prevCell->neighbors[prevCell->numNeighbors++] = GRID_INDEX_UNSAFE(x, y - 1);

				mMinTerrainSlope = std::min(mMinTerrainSlope, std::fabs(currEdge->heightDelta.z));
				mMaxTerrainSlope = std::max(mMaxTerrainSlope, std::fabs(currEdge->heightDelta.z));

				// NOTE:
				//  heightDelta is not actually a gradient vector-field!
				//  (vectors do not represent directions in world-space)
				mHeightDeltaVisData[idx * NUM_DIRS + dir] = currEdge->heightDelta * ((mSquareSize / mDownScale) >> 1);
			}

			if (y < numCellsZ - 1) {
				dir = DIR_S;

				currEdge = &currEdges[currCell->edges[dir]];
				prevEdge = &prevEdges[prevCell->edges[dir]];
				currNgb = &currCells[GRID_INDEX_UNSAFE(x, y + 1)];
				prevNgb = &prevCells[GRID_INDEX_UNSAFE(x, y + 1)];

				currEdge->heightDelta = vec3f(0.0f, 0.0f, (currNgb->height - currCell->height));
				prevEdge->heightDelta = vec3f(0.0f, 0.0f, (prevNgb->height - prevCell->height));

				currCell->neighbors[currCell->numNeighbors++] = GRID_INDEX_UNSAFE(x, y + 1);
				prevCell->neighbors[prevCell->numNeighbors++] = GRID_INDEX_UNSAFE(x, y + 1);

				mMinTerrainSlope = std::min(mMinTerrainSlope, std::fabs(currEdge->heightDelta.z));
				mMaxTerrainSlope = std::max(mMaxTerrainSlope, std::fabs(currEdge->heightDelta.z));

				mHeightDeltaVisData[idx * NUM_DIRS + dir] = currEdge->heightDelta * ((mSquareSize / mDownScale) >> 1);
			}

			if (x > 0) {
				dir = DIR_W;

				currEdge = &currEdges[currCell->edges[dir]];
				prevEdge = &prevEdges[prevCell->edges[dir]];
				currNgb = &currCells[GRID_INDEX_UNSAFE(x - 1, y)];
				prevNgb = &prevCells[GRID_INDEX_UNSAFE(x - 1, y)];

				currEdge->heightDelta = vec3f((currNgb->height - currCell->height), 0.0f, 0.0f);
				prevEdge->heightDelta = vec3f((prevNgb->height - prevCell->height), 0.0f, 0.0f);

				currCell->neighbors[currCell->numNeighbors++] = GRID_INDEX_UNSAFE(x - 1, y);
				prevCell->neighbors[prevCell->numNeighbors++] = GRID_INDEX_UNSAFE(x - 1, y);

				mMinTerrainSlope = std::min(mMinTerrainSlope, std::fabs(currEdge->heightDelta.x));
				mMaxTerrainSlope = std::max(mMaxTerrainSlope, std::fabs(currEdge->heightDelta.x));

				mHeightDeltaVisData[idx * NUM_DIRS + dir] = currEdge->heightDelta * ((mSquareSize / mDownScale) >> 1);
			}

			if (x < numCellsX - 1) {
				dir = DIR_E;

				currEdge = &currEdges[currCell->edges[dir]];
				prevEdge = &prevEdges[prevCell->edges[dir]];
				currNgb = &currCells[GRID_INDEX_UNSAFE(x + 1, y)];
				prevNgb = &prevCells[GRID_INDEX_UNSAFE(x + 1, y)];

				currEdge->heightDelta = vec3f((currNgb->height - currCell->height), 0.0f, 0.0f);
				prevEdge->heightDelta = vec3f((prevNgb->height - prevCell->height), 0.0f, 0.0f);

				currCell->neighbors[currCell->numNeighbors++] = GRID_INDEX_UNSAFE(x + 1, y);
				prevCell->neighbors[prevCell->numNeighbors++] = GRID_INDEX_UNSAFE(x + 1, y);

				mMinTerrainSlope = std::min(mMinTerrainSlope, std::fabs(currEdge->heightDelta.x));
				mMaxTerrainSlope = std::max(mMaxTerrainSlope, std::fabs(currEdge->heightDelta.x));

				mHeightDeltaVisData[idx * NUM_DIRS + dir] = currEdge->heightDelta * ((mSquareSize / mDownScale) >> 1);
			}
		}
	}

	if (mFlatTerrain) {
		PFFG_ASSERT((mMaxTerrainSlope - mMinTerrainSlope) < EPSILON);
	}
}

void CCGrid::Reset() {
	std::vector<Cell>& currCells = mGridStates[mCurrBufferIdx].cells;
	std::vector<Cell>& prevCells = mGridStates[mPrevBufferIdx].cells;

	// undo last frame's dynamic-global data writes
	for (std::set<unsigned int>::const_iterator it = mTouchedCells.begin(); it != mTouchedCells.end(); ++it) {
		const unsigned int idx = *it;

		Cell* currCell = &currCells[idx];
		Cell* prevCell = &prevCells[idx];

		currCell->ResetGlobalDynamicVars();
		prevCell->ResetGlobalDynamicVars();

		mDensityVisData[idx]     = 0.0f;
		mDiscomfortVisData[idx]  = currCell->staticDiscomfort;
		mAvgVelocityVisData[idx] = NVECf;
	}

	mTouchedCells.clear();
}






void CCGrid::AddGlobalDynamicCellData(
	std::vector<Cell>& currCells,
	std::vector<Cell>& prevCells,
	const Cell* cell,
	int cellsInRadius,
	const vec3f& vel,
	unsigned int type
) {
	// if objects are tightly clustered, they project one
	// large density blob and inter-weaving lanes are much
	// less likely to form when groups approach one another
	// (typically a small number of wider "tracks" appear)
	// cellsInRadius = 0;

	for (int x = -cellsInRadius; x <= cellsInRadius; x++) {
		for (int z = -cellsInRadius; z <= cellsInRadius; z++) {
			const int cx = int(cell->x) + x;
			const int cz = int(cell->y) + z;

			if (cx < 0 || cx >= int(numCellsX)) { continue; }
			if (cz < 0 || cz >= int(numCellsZ)) { continue; }

			if ((x * x) + (z * z) > (cellsInRadius * cellsInRadius)) {
				continue;
			}

			Cell* cf = &currCells[ GRID_INDEX_UNSAFE(cx, cz) ];
			Cell* cb = &prevCells[ GRID_INDEX_UNSAFE(cx, cz) ];

			switch (type) {
				case DATATYPE_DENSITY: {
					// a unit's density contribution must be *at least* equal
					// to the threshold value rho_bar within (the cells of) a
					// bounding disc of radius r, but *at most* rho_min or the
					// result will be self-obstruction
					// NOTE: now we require cells that are much larger than
					// units in order for local density to exceed rho_max or
					// even rho_min
					// NOTE: this produces the "sharp density discontinuities"
					// because cells can go from rho=0 to rho>=rho_max in one
					// frame due to unit movement ==> need some way to "shift" 
					// density based on unit's position within the center cell
					// NOTE: when rho_bar is always <= rho_min, how can we get
					// avoidance behavior around a *single* non-moving object?
					//
					// two contradictory requirements, regardless of cell-size:
					//     1) units must contribute a minimum amount of density so other units avoid them
					//     2) units must contribute a maximum amount of density so they do not self-impede
					// const float scale = 1.0f - ((std::abs(x) + std::abs(z)) / float(cellsInRadius << 1));
					// const float rho = mRhoBar + ((mRhoMin - mRhoBar - EPSILON) * scale);

					// if (vel.sqLen2D() <= EPSILON) { rho = mRhoMax; }
					// if (x == 0 && z == 0) { rho = mRhoMax; }

					cf->density += mRhoBar;
					cb->density += mRhoBar;
					cf->avgVelocity += (vel * mRhoBar);
					cb->avgVelocity += (vel * mRhoBar);
				} break;
				case DATATYPE_DISCOMFORT: {
					// scale the discomfort vector
					//
					// NOTE; this causes opposing velocity vectors
					// projected onto the same cell to cancel out,
					// so we store the total discomfort value in y
					// cf->mobileDiscomfort += (vel * mRhoBar);
					// cb->mobileDiscomfort += (vel * mRhoBar);

					// we require only the direction along xz
					// cf->mobileDiscomfort.x += (vel.x * mRhoBar);
					// cf->mobileDiscomfort.z += (vel.z * mRhoBar);
					// cf->mobileDiscomfort.y += (vel.len2D() * mRhoBar);

					// x and z are normalised later
					cf->mobileDiscomfort.x += vel.x;
					cf->mobileDiscomfort.z += vel.z;
					cf->mobileDiscomfort.y += mRhoBar;

					cb->mobileDiscomfort = cf->mobileDiscomfort;
				} break;
				default: {
				} break;
			}

			mTouchedCells.insert(GRID_INDEX_UNSAFE(cx, cz));
		}
	}
}

void CCGrid::AddDensity(const vec3f& pos, const vec3f& vel, float radius) {
	std::vector<Cell>& currCells = mGridStates[mCurrBufferIdx].cells;
	std::vector<Cell>& prevCells = mGridStates[mPrevBufferIdx].cells;

	const Cell* cell = &currCells[ GetCellIndex1D(pos) ];

	AddGlobalDynamicCellData(currCells, prevCells, cell, CELLS_IN_RADIUS(radius), vel, DATATYPE_DENSITY);
}

void CCGrid::AddDiscomfort(const vec3f& pos, const vec3f& vel, float radius, unsigned int numFrames, float stepSize) {
	if (vel.sqLen2D() <= EPSILON) {
		// no predictive discomfort for stationary objects
		// (density alone should cause those to be avoided)
		return;
	}

	std::vector<Cell>& currCells = mGridStates[mCurrBufferIdx].cells;
	std::vector<Cell>& prevCells = mGridStates[mPrevBufferIdx].cells;

	// NOTE:
	//    stepSize seems to be crucial for 4-way vortex formation
	//    but its use here is incorrect (discomfort from a single
	//    unit can now extend across the entire map, yet vortices
	//    fail to show up without this ==> center of the map needs
	//    to become a no-go discomfort zone such that every group
	//    will start to circle around it)
	// const unsigned int posCellIdx = GetCellIndex1D(pos);
	// stepSize = 1.0f;

	for (unsigned int n = 0; n <= numFrames; n++) {
		const vec3f        stepPos = pos + vel * n * stepSize;
		const unsigned int cellIdx = GetCellIndex1D(stepPos);
		const Cell*        cell    = &currCells[cellIdx];

		// skip our own cell
		// if (cellIdx == posCellIdx) { continue; }
		AddGlobalDynamicCellData(currCells, prevCells, cell, CELLS_IN_RADIUS(radius), vel, DATATYPE_DISCOMFORT);
	}
}



void CCGrid::ComputeAvgVelocity() {
	std::vector<Cell>& currCells = mGridStates[mCurrBufferIdx].cells;
	std::vector<Cell>& prevCells = mGridStates[mPrevBufferIdx].cells;

	for (std::set<unsigned int>::const_iterator it = mTouchedCells.begin(); it != mTouchedCells.end(); ++it) {
		const unsigned int idx = *it;

		Cell* cf = &currCells[idx];
		Cell* cb = &prevCells[idx];

		// v(i) is multiplied by rho(i) when summing v-bar,
		// so we need to divide by the non-normalised rho
		// to get the final per-cell average velocity
		if (cf->density > EPSILON) {
			cf->avgVelocity /= cf->density;
			cb->avgVelocity  = cf->avgVelocity;
		}
		// normalise xz to get the mobile discomfort direction
		// (should be more or less equal to avgVelocity, static
		// discomfort is assumed to already be normalised)
		if (cf->mobileDiscomfort.sqLen2D() > EPSILON) {
			cf->mobileDiscomfort = cf->mobileDiscomfort.norm2D();
			cb->mobileDiscomfort = cf->mobileDiscomfort;
		}

		// note: unnecessary? (density is only used to
		// decide between topological and flow speed)
		cf->density = CLAMP(cf->density, 0.0f, mRhoMax + EPSILON);
		cb->density = CLAMP(cb->density, 0.0f, mRhoMax + EPSILON);

		// normalise the cell densities, so that comparisons with
		// mRhoMin and mRhoMax are well-defined when constructing
		// the speed-field ==> already ensured regardless of the
		// rho_* scale
		//
		// NOTE:
		//     we do not even want normalisation
		//     eg. in the case of DENSITY_CONVERSION_TCP06 being 0,
		//     we add rho_bar density to each cell within a unit's
		//     density circle ==> if there is only one unit present,
		//     the normalised density would become 1 which exceeds
		//     rho_max (since the rho_* values still lie in [0, 1])
		// cf->density /= mMaxDensity;
		// cb->density  = cf->density;

		// NOTE: this is probably not what we want, since
		// more crowded cells now automatically receive a
		// lower average velocity even when every unit in
		// them is heading in the same direction
		// (it also does not make much sense, given that
		// v-bar is a summation of v(i) * rho(i) terms)
		//
		// cf->avgVelocity *= (1.0f - cf->density);
		// cb->avgVelocity  = cf->avgVelocity;

		mDensityVisData[idx]     = cf->density;
		mDiscomfortVisData[idx]  = cf->staticDiscomfort + cf->mobileDiscomfort;
		mAvgVelocityVisData[idx] = cf->avgVelocity;
	}
}






#if (SPEED_COST_POTENTIAL_MERGED_COMPUTATION == 0)
/*
	void CCGrid::ComputeCellSpeed(unsigned int groupID, unsigned int cellIdx, std::vector<Cell>& currCells, std::vector<Cell::Edge>& currEdges) {
		Cell* currCell = &currCells[cellIdx];

		const unsigned int densityDirOffset = CELLS_IN_RADIUS(mMaxGroupRadius) + 1;

		for (unsigned int dir = 0; dir < NUM_DIRS; dir++) {
			const unsigned int densityDirIndex = GRID_INDEX_CLAMPED(
				currCell->x + mDirDeltas[dir].x * densityDirOffset,
				currCell->y + mDirDeltas[dir].z * densityDirOffset);

			const Cell*       currCellDirNgb  = &currCells[densityDirIndex];
			const Cell::Edge* currCellDirEdge = &currEdges[currCell->edges[dir]];

			const float cellDirSlope    = currCellDirEdge->heightDelta.dot2D(mDirVectors[dir]);
			      float cellDirSlopeMod = 0.0f;

			// f_{M --> dir} for computing f, based on offset density
			float cellDirSpeed = 0.0f;

			{
				if (POSITIVE_SLOPE(dir, cellDirSlope)) { cellDirSlopeMod =  std::fabs(cellDirSlope); }
				if (NEGATIVE_SLOPE(dir, cellDirSlope)) { cellDirSlopeMod = -std::fabs(cellDirSlope); }

				const float densityDirSpeedScale = (currCellDirNgb->density - mRhoMin) / (mRhoMax - mRhoMin);
				const float slopeDirSpeedScale   = mFlatTerrain? 0.0f: ((cellDirSlopeMod - mMinTerrainSlope) / (mMaxTerrainSlope - mMinTerrainSlope));
				const float cellDirTopoSpeed     = mMaxGroupSpeed + CLAMP(slopeDirSpeedScale, -1.0f, 1.0f) * (mMinGroupSpeed - mMaxGroupSpeed);
				const float cellDirFlowSpeed     = std::max(0.0f, currCellDirNgb->avgVelocity.dot2D(mDirVectors[dir]));
				const float cellDirTopoFlowSpeed = cellDirTopoSpeed + densityDirSpeedScale * (cellDirTopoSpeed - cellDirFlowSpeed);

				cellDirSpeed = cellDirTopoFlowSpeed;

				if (currCellDirNgb->density >= mRhoMax) { cellDirSpeed = cellDirFlowSpeed; }
				if (currCellDirNgb->density <= mRhoMin) { cellDirSpeed = cellDirTopoSpeed; }
			}

			currCell->speed[dir] = cellDirSpeed;
			mSpeedVisData[groupID][cellIdx * NUM_DIRS + dir] = cellDirSpeed;
		}
	}

	void CCGrid::ComputeCellCost(unsigned int groupID, unsigned int cellIdx, std::vector<Cell>& currCells, std::vector<Cell::Edge>& currEdges) {
		Cell* currCell = &currCells[cellIdx];

		for (unsigned int dir = 0; dir < NUM_DIRS; dir++) {
			const Cell::Edge* currCellDirEdge = &currEdges[currCell->edges[dir]];
			const Cell*       currCellDirNgb  = NULL;

			switch (dir) {
				case DIR_N: { currCellDirNgb = (currCell->y >             0)? &currCells[GRID_INDEX_UNSAFE(currCell->x,     currCell->y - 1)]: currCell; } break;
				case DIR_S: { currCellDirNgb = (currCell->y < numCellsZ - 1)? &currCells[GRID_INDEX_UNSAFE(currCell->x,     currCell->y + 1)]: currCell; } break;
				case DIR_E: { currCellDirNgb = (currCell->x < numCellsX - 1)? &currCells[GRID_INDEX_UNSAFE(currCell->x + 1, currCell->y    )]: currCell; } break;
				case DIR_W: { currCellDirNgb = (currCell->x >             0)? &currCells[GRID_INDEX_UNSAFE(currCell->x - 1, currCell->y    )]: currCell; } break;
			}

			const float cellDirDiscomfort = currCellDirNgb->staticDiscomfort.y + currCellDirNgb->mobileDiscomfort.y;
			const float cellDirSlope      = currCellDirEdge->heightDelta.dot2D(mDirVectors[dir]);
			      float cellDirSlopeMod   = 0.0f;

			float cellDirSpeed = 0.0f; // f_{M --> dir} for computing C, based on direct neighbor density
			float cellDirCost  = 0.0f; // C_{M --> dir}

			{
				if (POSITIVE_SLOPE(dir, cellDirSlope)) { cellDirSlopeMod =  std::fabs(cellDirSlope); }
				if (NEGATIVE_SLOPE(dir, cellDirSlope)) { cellDirSlopeMod = -std::fabs(cellDirSlope); }

				const float densityDirSpeedScale = (currCellDirNgb->density - mRhoMin) / (mRhoMax - mRhoMin);
				const float slopeDirSpeedScale   = mFlatTerrain? 0.0f: ((cellDirSlopeMod - mMinTerrainSlope) / (mMaxTerrainSlope - mMinTerrainSlope));
				const float cellDirTopoSpeed     = mMaxGroupSpeed + CLAMP(slopeDirSpeedScale, -1.0f, 1.0f) * (mMinGroupSpeed - mMaxGroupSpeed);
				const float cellDirFlowSpeed     = std::max(0.0f, currCellDirNgb->avgVelocity.dot2D(mDirVectors[dir]));
				const float cellDirTopoFlowSpeed = cellDirTopoSpeed + densityDirSpeedScale * (cellDirTopoSpeed - cellDirFlowSpeed);

				cellDirSpeed = cellDirTopoFlowSpeed;

				if (currCellDirNgb->density >= mRhoMax) { cellDirSpeed = cellDirFlowSpeed; }
				if (currCellDirNgb->density <= mRhoMin) { cellDirSpeed = cellDirTopoSpeed; }

				if (cellDirSpeed > EPSILON) {
					cellDirCost = ((mAlphaWeight * cellDirSpeed) + mBetaWeight + (mGammaWeight * cellDirDiscomfort)) / cellDirSpeed;
				} else {
					cellDirSpeed = EPSILON;
					cellDirCost = ((mAlphaWeight * cellDirSpeed) + mBetaWeight + (mGammaWeight * cellDirDiscomfort)) / cellDirSpeed;
				}
			}

			currCell->cost[dir] = cellDirCost;
			mCostVisData[groupID][cellIdx * NUM_DIRS + dir] = cellDirCost;
		}
	}
	// ComputeCellSpeedAndCost() falls within the #if branch
	// as well, but is re-used when MERGED == 1 so we always
	// compile it
*/
#endif



// compute the speed- and unit-cost fields, per cell
//
// NOTE:
//    if the flow-speed is zero in a region, then the cost for
//    cells with normalised density >= mRhoMax will be infinite
//    everywhere and the potential-field group update triggers
//    asserts; this happens for cells with density <= mRhoMin
//    whenever topoSpeed is less than or equal to zero as well
//
void CCGrid::ComputeCellSpeedAndCost(unsigned int groupID, unsigned int cellIdx, std::vector<Cell>& currCells, std::vector<Cell::Edge>& currEdges) {
	Cell* currCell = &currCells[cellIdx];

	// this assumes a unit's contribution to the density field
	// is no greater than rho-bar outside a disc of radius <r>
	// (such that f == f_topological when rho_min >= rho_bar)
	//
	// however, when mMaxGroupRadius < (mSquareSize >> 1) this
	// just maps to <currCell>, hence the CELLS_IN_RADIUS macro
	// (which ensures a minimum offset of 1 cell) is used here
	// instead
	//
	// const vec3f& cellPos = GetCellMidPos(currCell);
	// const vec3f densityDirOffset = mDirVectors[dir] * (mMaxGroupRadius + EPSILON);
	// const Cell* currCellDirNgbR = &currCells[ GetCellIndex1D(cellPos + densityDirOffset) ];
	//
	// add one cell so we always sample outside a unit's disc
	const unsigned int densityDirOffset = CELLS_IN_RADIUS(mMaxGroupRadius) + 1;

	for (unsigned int dir = 0; dir < NUM_DIRS; dir++) {
		const unsigned int densityDirIndex = GRID_INDEX_CLAMPED(
			currCell->x + mDirDeltas[dir].x * densityDirOffset,
			currCell->y + mDirDeltas[dir].z * densityDirOffset);

		const Cell*       currCellDirNgbR = &currCells[densityDirIndex];
		const Cell*       currCellDirNgbC = NULL;
		const Cell::Edge* currCellDirEdge = &currEdges[currCell->edges[dir]];

		#if (SPEED_COST_SHARED_NEIGHBOR_CELL == 0)
			switch (dir) {
				case DIR_N: { currCellDirNgbC = (currCell->y >             0)? &currCells[GRID_INDEX_UNSAFE(currCell->x,     currCell->y - 1)]: currCell; } break;
				case DIR_S: { currCellDirNgbC = (currCell->y < numCellsZ - 1)? &currCells[GRID_INDEX_UNSAFE(currCell->x,     currCell->y + 1)]: currCell; } break;
				case DIR_E: { currCellDirNgbC = (currCell->x < numCellsX - 1)? &currCells[GRID_INDEX_UNSAFE(currCell->x + 1, currCell->y    )]: currCell; } break;
				case DIR_W: { currCellDirNgbC = (currCell->x >             0)? &currCells[GRID_INDEX_UNSAFE(currCell->x - 1, currCell->y    )]: currCell; } break;
			}
		#else
			// use the same neighbor (to sample density,
			// discomfort, and vAvg from) for C as for f
			currCellDirNgbC = currCellDirNgbR;
		#endif

		const float cellDirSlope      = currCellDirEdge->heightDelta.dot2D(mDirVectors[dir]);
		      float cellDirSlopeMod   = 0.0f;
		      float cellDirDiscomfort = 0.0f;

		#if (SPEED_COST_DIRECTIONAL_DISCOMFORT == 1)
			// negate the dot-product and map it from [-1.0, 1.0] to [0.0, 1.0]
			//   dot  1.0 (parallel) ==> minimal discomfort contribution to cost ==> scale ((( 1.0 * -1.0) + 1.0) * 0.5) = 0.0
			//   dot  0.0 (orthogon) ==> medium  discomfort contribution to cost ==> scale ((( 0.0 * -1.0) + 1.0) * 0.5) = 0.5
			//   dot -1.0 (opposite) ==> maximum discomfort contribution to cost ==> scale (((-1.0 * -1.0) + 1.0) * 0.5) = 1.0
			const float staticDiscomfortScale = ((currCellDirNgbC->staticDiscomfort.dot2D(mDirVectors[dir]) * -1.0f) + 1.0f) * 0.5f;
			const float mobileDiscomfortScale = ((currCellDirNgbC->mobileDiscomfort.dot2D(mDirVectors[dir]) * -1.0f) + 1.0f) * 0.5f;

			// for a unit moving in a direction parallel to a discomfort zone,
			// the discomfort inside the zone should still be slightly higher
			// than outside it (the zones should not act as attractors, though
			// this produces more pronounced lanes)?
			// FIXME: one group moving SE and another moving SW produce a net
			// discomfort direction of S, which is only misaligned 45 degrees
			// with their own (thus both experience minimal discomfort value)
			// can be partially offset with more (hundreds) prediction frames
			cellDirDiscomfort =
				currCellDirNgbC->staticDiscomfort.y * staticDiscomfortScale +
				currCellDirNgbC->mobileDiscomfort.y * mobileDiscomfortScale;
		#else
			cellDirDiscomfort =
				currCellDirNgbC->staticDiscomfort.y +
				currCellDirNgbC->mobileDiscomfort.y;
		#endif

		float cellDirSpeedR = 0.0f; // f_{M --> dir} for computing f, based on offset density (R=RHO)
		float cellDirSpeedC = 0.0f; // f_{M --> dir} for computing C, based on direct neighbor density (C=COST)
		float cellDirCost   = 0.0f; // C_{M --> dir}

		{
			// if the slope is positive along <dir>, we want a positive slopeSpeedScale
			// if the slope is negative along <dir>, we want a negative slopeSpeedScale
			//
			// since (s_max - s_min) is always positive and (f_min - f_max) is always
			// negative, the numerator (s - s_min) must be greater than 0 to achieve a
			// speed-decrease on positive slopes and smaller than 0 to achieve a speed-
			// increase on negative slopes
			if (POSITIVE_SLOPE(dir, cellDirSlope)) { cellDirSlopeMod =  std::fabs(cellDirSlope); }
			if (NEGATIVE_SLOPE(dir, cellDirSlope)) { cellDirSlopeMod = -std::fabs(cellDirSlope); }

			const float densityDirSpeedScaleR = (currCellDirNgbR->density - mRhoMin) / (mRhoMax - mRhoMin);
			const float densityDirSpeedScaleC = (currCellDirNgbC->density - mRhoMin) / (mRhoMax - mRhoMin);

			const float slopeDirSpeedScale   = mFlatTerrain? 0.0f: ((cellDirSlopeMod - mMinTerrainSlope) / (mMaxTerrainSlope - mMinTerrainSlope));
			const float cellDirTopoSpeed     = mMaxGroupSpeed + CLAMP(slopeDirSpeedScale, -1.0f, 1.0f) * (mMinGroupSpeed - mMaxGroupSpeed);

			const float cellDirFlowSpeedR = std::max(0.0f, currCellDirNgbR->avgVelocity.dot2D(mDirVectors[dir]));
			const float cellDirFlowSpeedC = std::max(0.0f, currCellDirNgbC->avgVelocity.dot2D(mDirVectors[dir]));

			const float cellDirTopoFlowSpeedR = cellDirTopoSpeed + densityDirSpeedScaleR * (cellDirTopoSpeed - cellDirFlowSpeedR);
			const float cellDirTopoFlowSpeedC = cellDirTopoSpeed + densityDirSpeedScaleC * (cellDirTopoSpeed - cellDirFlowSpeedC);

			// default to linear interpolation of topological and flow speed
			cellDirSpeedR = cellDirTopoFlowSpeedR;
			cellDirSpeedC = cellDirTopoFlowSpeedC;

			if (currCellDirNgbR->density >= mRhoMax) { cellDirSpeedR = cellDirFlowSpeedR; }
			if (currCellDirNgbR->density <= mRhoMin) { cellDirSpeedR = cellDirTopoSpeed;  }

			if (currCellDirNgbC->density >= mRhoMax) { cellDirSpeedC = cellDirFlowSpeedC; }
			if (currCellDirNgbC->density <= mRhoMin) { cellDirSpeedC = cellDirTopoSpeed;  }

			if (cellDirSpeedC > EPSILON) {
				cellDirCost = ((mAlphaWeight * cellDirSpeedC) + mBetaWeight + (mGammaWeight * cellDirDiscomfort)) / (cellDirSpeedC * cellDirSpeedC);
			} else {
				// should this case be allowed to happen?
				// (infinite costs very heavily influence
				// behavior of potential-field generation)
				//
				// cost = std::numeric_limits<float>::infinity();
				// cost = std::numeric_limits<float>::max();
				cellDirSpeedC = EPSILON;
				cellDirCost = ((mAlphaWeight * cellDirSpeedC) + mBetaWeight + (mGammaWeight * cellDirDiscomfort)) / (cellDirSpeedC * cellDirSpeedC);
			}
		}

		currCell->speed[dir] = cellDirSpeedR;
		currCell->cost[dir] = cellDirCost;

		mSpeedVisData[groupID][cellIdx * NUM_DIRS + dir] = cellDirSpeedR;
		mCostVisData[groupID][cellIdx * NUM_DIRS + dir] = cellDirCost;
	}
}



#if (SPEED_COST_POTENTIAL_MERGED_COMPUTATION == 1)
	void CCGrid::ComputeCellSpeedAndCostMERGED(unsigned int groupID, Cell* currCell, std::vector<Cell>& currCells, std::vector<Cell::Edge>& currEdges) {
		// recycled from (MERGED == 0 && SINGLE_PASS == 1)
		ComputeCellSpeedAndCost(groupID, GRID_INDEX_UNSAFE(currCell->x, currCell->y), currCells, currEdges);
	}
#else
	/*
	void CCGrid::ComputeSpeedAndCost(unsigned int groupID) {
		std::vector<Cell      >& currCells = mGridStates[mCurrBufferIdx].cells;
		std::vector<Cell::Edge>& currEdges = mGridStates[mCurrBufferIdx].edges;

		#if (SPEED_COST_SINGLE_PASS_COMPUTATION == 1)
			for (unsigned int cellIdx = 0; cellIdx < (numCellsX * numCellsZ); cellIdx++) {
				ComputeCellSpeedAndCost(groupID, cellIdx, currCells, currEdges);
			}
		#else
			for (unsigned int cellIdx = 0; cellIdx < (numCellsX * numCellsZ); cellIdx++) { ComputeCellSpeed(groupID, cellIdx, currCells, currEdges); }
			for (unsigned int cellIdx = 0; cellIdx < (numCellsX * numCellsZ); cellIdx++) { ComputeCellCost(groupID, cellIdx, currCells, currEdges); }
		#endif
	}
	*/
#endif






void CCGrid::UpdateGroupPotentialField(unsigned int groupID, const std::set<unsigned int>& goalIDs, const std::set<unsigned int>& objectIDs) {
	PFFG_ASSERT(!goalIDs.empty());
	PFFG_ASSERT(mCandidates.empty());

	// cycle the buffers so the per-group variables of the
	// previously processed group do not influence this one
	//
	// NOTE: this must be done here rather than at the end
	// of the update so that UpdateSimObjectLocation still
	// reads from the correct buffer
	mCurrBufferIdx = (mCurrBufferIdx + 1) & 1;
	mPrevBufferIdx = (mPrevBufferIdx + 1) & 1;

	{
		mMinGroupSlope  =  std::numeric_limits<float>::max();
		mMaxGroupSlope  = -std::numeric_limits<float>::max();
		mMinGroupSpeed  =                               0.0f;
		mMaxGroupSpeed  = -std::numeric_limits<float>::max();
		mMaxGroupRadius = -std::numeric_limits<float>::max();

		for (std::set<unsigned int>::iterator i = objectIDs.begin(); i != objectIDs.end(); i++) {
			const SimObjectDef* simObjectDef = mCOH->GetSimObjectDef(*i);

			mMinGroupSlope  = std::min<float>(mMinGroupSlope,  simObjectDef->GetMinSlopeAngleCosine());
			mMaxGroupSlope  = std::max<float>(mMaxGroupSlope,  simObjectDef->GetMaxSlopeAngleCosine());
			mMaxGroupSpeed  = std::max<float>(mMaxGroupSpeed,  simObjectDef->GetMaxForwardSpeed());
			mMaxGroupRadius = std::max<float>(mMaxGroupRadius, mCOH->GetSimObjectRadius(*i));
		}
	}

	#if (SPEED_COST_POTENTIAL_MERGED_COMPUTATION == 0)
	ComputeSpeedAndCost(groupID);
	#endif

	Buffer& currGridBuffer = mGridStates[mCurrBufferIdx];
	Buffer& prevGridBuffer = mGridStates[mPrevBufferIdx];
	Buffer& groupGridState = mGroupGridStates[groupID];

	std::vector<float>& potVisData      = mPotentialVisData[groupID];
	std::vector<vec3f>& velVisData      = mVelocityVisData[groupID];
	std::vector<vec3f>& potDeltaVisData = mPotentialDeltaVisData[groupID];

	std::vector<Cell      >& currCells = currGridBuffer.cells;
	std::vector<Cell      >& prevCells = prevGridBuffer.cells;
	std::vector<Cell::Edge>& currEdges = currGridBuffer.edges;
	std::vector<Cell::Edge>& prevEdges = prevGridBuffer.edges;

	Cell* currCell = NULL;
	Cell* prevCell = NULL;

	numInfinitePotentialCases = 0;
	numIllegalDirectionCases = 0;

	unsigned int cellIdx = 0;

	// add goal-cells to the known set and their neighbors to the candidate-set
	for (std::set<unsigned int>::const_iterator it = goalIDs.begin(); it != goalIDs.end(); ++it) {
		cellIdx = *it;

		currCell = &currCells[cellIdx];
		prevCell = &prevCells[cellIdx];

		currCell->known = true;
		currCell->candidate = true;
		currCell->potential = 0.0f;
		prevCell->ResetGroupVars();

		#if (SPEED_COST_POTENTIAL_MERGED_COMPUTATION == 1)
		// <currCell> is a goal-cell, so this is not necessary?
		// ComputeCellSpeedAndCostMERGED(groupID, currCell, currCells, currEdges);
		#endif
		UpdateCandidates(groupID, currCell);

		potVisData[cellIdx] = (currCell->potential == std::numeric_limits<float>::infinity())? -1.0f: currCell->potential;
		velVisData[cellIdx * NUM_DIRS + DIR_N] = NVECf;
		velVisData[cellIdx * NUM_DIRS + DIR_S] = NVECf;
		velVisData[cellIdx * NUM_DIRS + DIR_E] = NVECf;
		velVisData[cellIdx * NUM_DIRS + DIR_W] = NVECf;
		potDeltaVisData[cellIdx * NUM_DIRS + DIR_N] = NVECf;
		potDeltaVisData[cellIdx * NUM_DIRS + DIR_S] = NVECf;
		potDeltaVisData[cellIdx * NUM_DIRS + DIR_E] = NVECf;
		potDeltaVisData[cellIdx * NUM_DIRS + DIR_W] = NVECf;
	}

	while (!mCandidates.empty()) {
		currCell = mCandidates.top();
		currCell->known = true;

		cellIdx = GRID_INDEX_UNSAFE(currCell->x, currCell->y);

		prevCell = &prevCells[cellIdx];
		prevCell->ResetGroupVars();

		UpdateCandidates(groupID, currCell);

		currEdges[ currCell->edges[DIR_N] ].velocity = (currCell->GetNormalisedPotentialGradient(currEdges, DIR_N) * -currCell->speed[DIR_N]);
		currEdges[ currCell->edges[DIR_S] ].velocity = (currCell->GetNormalisedPotentialGradient(currEdges, DIR_S) * -currCell->speed[DIR_S]);
		currEdges[ currCell->edges[DIR_E] ].velocity = (currCell->GetNormalisedPotentialGradient(currEdges, DIR_E) * -currCell->speed[DIR_E]);
		currEdges[ currCell->edges[DIR_W] ].velocity = (currCell->GetNormalisedPotentialGradient(currEdges, DIR_W) * -currCell->speed[DIR_W]);
		prevEdges[ currCell->edges[DIR_N] ].velocity = NVECf;
		prevEdges[ currCell->edges[DIR_S] ].velocity = NVECf;
		prevEdges[ currCell->edges[DIR_E] ].velocity = NVECf;
		prevEdges[ currCell->edges[DIR_W] ].velocity = NVECf;

		if (mUpdateInt > 1) {
			// the grid-state gets recycled for the next group,
			// so when mUpdateInt is not 1 we need to keep the
			// values cached during the non-update frames
			groupGridState.cells[cellIdx] = *currCell;
			groupGridState.edges[ currCell->edges[DIR_N] ] = currEdges[ currCell->edges[DIR_N]];
			groupGridState.edges[ currCell->edges[DIR_S] ] = currEdges[ currCell->edges[DIR_S]];
			groupGridState.edges[ currCell->edges[DIR_E] ] = currEdges[ currCell->edges[DIR_E]];
			groupGridState.edges[ currCell->edges[DIR_W] ] = currEdges[ currCell->edges[DIR_W]];
		}

		potVisData[cellIdx] = (currCell->potential == std::numeric_limits<float>::infinity())? -1.0f: currCell->potential;
		velVisData[cellIdx * NUM_DIRS + DIR_N] = currEdges[ currCell->edges[DIR_N] ].velocity * (mSquareSize >> 1);
		velVisData[cellIdx * NUM_DIRS + DIR_S] = currEdges[ currCell->edges[DIR_S] ].velocity * (mSquareSize >> 1);
		velVisData[cellIdx * NUM_DIRS + DIR_E] = currEdges[ currCell->edges[DIR_E] ].velocity * (mSquareSize >> 1);
		velVisData[cellIdx * NUM_DIRS + DIR_W] = currEdges[ currCell->edges[DIR_W] ].velocity * (mSquareSize >> 1);
		potDeltaVisData[cellIdx * NUM_DIRS + DIR_N] = currEdges[ currCell->edges[DIR_N] ].potentialDelta * (mSquareSize >> 1);
		potDeltaVisData[cellIdx * NUM_DIRS + DIR_S] = currEdges[ currCell->edges[DIR_S] ].potentialDelta * (mSquareSize >> 1);
		potDeltaVisData[cellIdx * NUM_DIRS + DIR_E] = currEdges[ currCell->edges[DIR_E] ].potentialDelta * (mSquareSize >> 1);
		potDeltaVisData[cellIdx * NUM_DIRS + DIR_W] = currEdges[ currCell->edges[DIR_W] ].potentialDelta * (mSquareSize >> 1);

		mCandidates.pop();
	}

	PFFG_ASSERT(numInfinitePotentialCases == 0 && numIllegalDirectionCases == 0);
}

void CCGrid::UpdateCandidates(unsigned int groupID, const Cell* parent) {
	Buffer& currGridBuffer = mGridStates[mCurrBufferIdx];
	Buffer& prevGridBuffer = mGridStates[mPrevBufferIdx];

	std::vector<Cell      >& currCells = currGridBuffer.cells;
	std::vector<Cell      >& prevCells = prevGridBuffer.cells;
	std::vector<Cell::Edge>& currEdges = currGridBuffer.edges;
	std::vector<Cell::Edge>& prevEdges = prevGridBuffer.edges;

	static float       dirCosts[NUM_DIRS] = {0.0f};
	static bool        dirValid[NUM_DIRS] = {false};
	static const Cell* dirCells[NUM_DIRS] = {NULL};

	const Cell* minPotCellPtrX = NULL;
	const Cell* minPotCellPtrY = NULL;
	int         minPotCellDirX = -1;
	int         minPotCellDirY = -1;

	Cell::Edge* currEdgeX = NULL;
	Cell::Edge* currEdgeY = NULL;
	Cell::Edge* prevEdgeX = NULL;
	Cell::Edge* prevEdgeY = NULL;

	for (unsigned int i = 0; i < parent->numNeighbors; i++) {
		const unsigned int ngbIdx = parent->neighbors[i];

		Cell* currNgb = &currCells[ngbIdx];
		Cell* prevNgb = &prevCells[ngbIdx];

		prevNgb->ResetGroupVars();

		if (currNgb->known || currNgb->candidate) {
			continue;
		}

		#if (SPEED_COST_POTENTIAL_MERGED_COMPUTATION == 1)
		ComputeCellSpeedAndCostMERGED(groupID, currNgb, currCells, currEdges);
		#else
		groupID = groupID;
		#endif

		dirCells[DIR_N] = (currNgb->y >             0) ? &currCells[GRID_INDEX_UNSAFE(currNgb->x    , currNgb->y - 1)] : NULL;
		dirCells[DIR_S] = (currNgb->y < numCellsZ - 1) ? &currCells[GRID_INDEX_UNSAFE(currNgb->x    , currNgb->y + 1)] : NULL;
		dirCells[DIR_E] = (currNgb->x < numCellsX - 1) ? &currCells[GRID_INDEX_UNSAFE(currNgb->x + 1, currNgb->y    )] : NULL;
		dirCells[DIR_W] = (currNgb->x >             0) ? &currCells[GRID_INDEX_UNSAFE(currNgb->x - 1, currNgb->y    )] : NULL;

		for (unsigned int dir = 0; dir < NUM_DIRS; dir++) {
			if (dirCells[dir] == NULL) {
				dirValid[dir] = false;
				dirCosts[dir] = std::numeric_limits<float>::infinity();
			} else {
				dirCosts[dir] = (dirCells[dir]->potential + dirCells[dir]->cost[dir]);
				dirValid[dir] = (dirCosts[dir] != std::numeric_limits<float>::infinity());
			}
		}

		const bool undefinedX = (!dirValid[DIR_E] && !dirValid[DIR_W]);
		const bool undefinedY = (!dirValid[DIR_N] && !dirValid[DIR_S]);

		// at least one dimension must ALWAYS be defined
		PFFG_ASSERT((int(undefinedX) + int(undefinedY)) < 2);

		if (!undefinedX && !undefinedY) {
			// both dimensions are defined
			if (dirCosts[DIR_E] < dirCosts[DIR_W]) {
				minPotCellDirX = DIR_E;
				minPotCellPtrX = dirCells[DIR_E];
			} else {
				minPotCellDirX = DIR_W;
				minPotCellPtrX = dirCells[DIR_W];
			}

			if (dirCosts[DIR_N] < dirCosts[DIR_S]) {
				minPotCellDirY = DIR_N;
				minPotCellPtrY = dirCells[DIR_N];
			} else {
				minPotCellDirY = DIR_S;
				minPotCellPtrY = dirCells[DIR_S];
			}

			PFFG_ASSERT(dirValid[DIR_N] || dirValid[DIR_S]);
			PFFG_ASSERT(dirValid[DIR_E] || dirValid[DIR_W]);
			PFFG_ASSERT(minPotCellPtrX != NULL && minPotCellPtrY != NULL);

			if (minPotCellPtrX != NULL && minPotCellPtrY != NULL) {
				currNgb->potential = Potential2D(
					minPotCellPtrX->potential, currNgb->cost[minPotCellDirX], 
					minPotCellPtrY->potential, currNgb->cost[minPotCellDirY]
				);

				// the world-space direction of the gradient vector must always
				// match the direction along which the potential increases, but
				// for DIR_N and DIR_W these are inverted
				const float gradientX = (minPotCellPtrX->potential - currNgb->potential);
				const float gradientY = (minPotCellPtrY->potential - currNgb->potential);
				const float scaleX    = (minPotCellDirX == DIR_W)? -1.0f: 1.0f;
				const float scaleY    = (minPotCellDirY == DIR_N)? -1.0f: 1.0f;
				const vec3f gradient  = vec3f(gradientX * scaleX, 0.0f, gradientY * scaleY);

				currEdgeX = &currEdges[currNgb->edges[minPotCellDirX]];
				currEdgeY = &currEdges[currNgb->edges[minPotCellDirY]];
				prevEdgeX = &prevEdges[currNgb->edges[minPotCellDirX]];
				prevEdgeY = &prevEdges[currNgb->edges[minPotCellDirY]];

				currEdgeX->potentialDelta = gradient;
				currEdgeY->potentialDelta = gradient;
				prevEdgeX->potentialDelta = NVECf;
				prevEdgeY->potentialDelta = NVECf;
			} else {
				numIllegalDirectionCases += 1;
			}
		} else {
			if (undefinedX) {
				if (dirCosts[DIR_N] < dirCosts[DIR_S]) {
					minPotCellDirY = DIR_N;
					minPotCellPtrY = dirCells[DIR_N];
				} else {
					minPotCellDirY = DIR_S;
					minPotCellPtrY = dirCells[DIR_S];
				}

				PFFG_ASSERT(dirValid[DIR_N] || dirValid[DIR_S]);
				PFFG_ASSERT(!undefinedY);
				PFFG_ASSERT(minPotCellPtrY != NULL);

				if (minPotCellPtrY != NULL) {
					currNgb->potential = Potential1D(minPotCellPtrY->potential, currNgb->cost[minPotCellDirY]);

					const float gradientY = (minPotCellPtrY->potential - currNgb->potential);
					const float scaleY    = (minPotCellDirY == DIR_N)? -1.0f: 1.0f;
					const vec3f gradient  = vec3f(0.0f, 0.0f, gradientY * scaleY);

					currEdgeY = &currEdges[currNgb->edges[minPotCellDirY]];
					prevEdgeY = &prevEdges[currNgb->edges[minPotCellDirY]];

					currEdgeY->potentialDelta = gradient;
					prevEdgeY->potentialDelta = NVECf;
				} else {
					numIllegalDirectionCases += 1;
				}
			}

			if (undefinedY) {
				if (dirCosts[DIR_E] < dirCosts[DIR_W]) {
					minPotCellDirX = DIR_E;
					minPotCellPtrX = dirCells[DIR_E];
				} else {
					minPotCellDirX = DIR_W;
					minPotCellPtrX = dirCells[DIR_W];
				}

				PFFG_ASSERT(dirValid[DIR_E] || dirValid[DIR_W]);
				PFFG_ASSERT(!undefinedX);
				PFFG_ASSERT(minPotCellPtrX != NULL);

				if (minPotCellPtrX != NULL) {
					currNgb->potential = Potential1D(minPotCellPtrX->potential, currNgb->cost[minPotCellDirX]);

					const float gradientX = (minPotCellPtrX->potential - currNgb->potential);
					const float scaleX    = (minPotCellDirX == DIR_W)? -1.0f: 1.0f;
					const vec3f gradient  = vec3f(gradientX * scaleX, 0.0f, 0.0f);

					currEdgeX = &currEdges[currNgb->edges[minPotCellDirX]];
					prevEdgeX = &prevEdges[currNgb->edges[minPotCellDirX]];

					currEdgeX->potentialDelta = gradient;
					prevEdgeX->potentialDelta = NVECf;
				} else {
					numIllegalDirectionCases += 1;
				}
			}
		}

		currNgb->candidate = true;
		mCandidates.push(currNgb);

		numInfinitePotentialCases += int(currNgb->potential == std::numeric_limits<float>::infinity());
	}
}



float CCGrid::Potential1D(const float p, const float c) const {
	return std::max<float>(p + c, p - c);
}

float CCGrid::Potential2D(const float p1, const float c1, const float p2, const float c2) const {
	// (c1^2*p2 + c2^2*p1) / (c1^2+c2^2) +/- (c1*c2 / sqrt(c1^2 + c2^2))
	const float c1s = c1 * c1;
	const float c2s = c2 * c2;
	const float c1s_plus_c2s = CLAMP(c1s + c2s, EPSILON, std::numeric_limits<float>::max());

	PFFG_ASSERT(c1s_plus_c2s > 0.0f);

	const float a = (c1s * p2 + c2s * p1) / (c1s_plus_c2s);
	const float b = CLAMP(sqrtf(c1s_plus_c2s), EPSILON, std::numeric_limits<float>::max());
	const float c = (c1 * c2) / b;

	// FIXME:
	//     huge variance in potential values (==> excessive gradients) because
	//     because of division by EPSILON * EPSILON in ComputeCellSpeedAndCost
	return std::max<float>(a + c, a - c);
}



bool CCGrid::UpdateSimObjectLocation(unsigned int groupID, unsigned int objectID, unsigned int objectCellID) {
	const vec3f& objectPos = mCOH->GetSimObjectPosition(objectID);
	const vec3f& objectDir = mCOH->GetSimObjectDirection(objectID);

	const Buffer& buffer =
		(mUpdateInt > 1)?
		mGroupGridStates[groupID]:
		mGridStates[mCurrBufferIdx];

	const std::vector<Cell      >& currCells = buffer.cells;
	const std::vector<Cell::Edge>& currEdges = buffer.edges;


	const Cell* objectCell = &currCells[objectCellID];
	const vec3f& objectCellVel = GetInterpolatedVelocity(currEdges, objectCell, objectPos, objectDir);

	PFFG_ASSERT_MSG(!(std::isnan(objectCellVel.x) || std::isnan(objectCellVel.y) || std::isnan(objectCellVel.z)), "Inf velocity-field for cell <%u,%u>", objectCell->x, objectCell->y);
	PFFG_ASSERT_MSG(!(std::isinf(objectCellVel.x) || std::isinf(objectCellVel.y) || std::isinf(objectCellVel.z)), "NaN velocity-field for cell <%u,%u>", objectCell->x, objectCell->y);

	if (objectCellVel.sqLen2D() > EPSILON) {
		#if (VELOCITY_FIELD_DIRECT_INTERPOLATION == 1)
			mCOH->SetSimObjectRawPhysicalState(objectID, objectPos + objectCellVel, objectCellVel.norm3D(), objectCellVel.len2D());
		#else
			const SimObjectDef* objectDef = mCOH->GetSimObjectDef(objectID);

			const float objectSpeed = mCOH->GetSimObjectSpeed(objectID);
			const float maxAccRate = objectDef->GetMaxAccelerationRate();
			const float maxDecRate = objectDef->GetMaxDeccelerationRate();

			// in theory, the velocity-field should never cause units
			// in any group to exceed that group's speed limitations
			// (note that this is not true on slopes)
			// float wantedSpeed = std::min(objectCellVel.len2D(), objectDef->GetMaxForwardSpeed());
			float wantedSpeed = objectCellVel.len2D();

			// note: should accelerate and deccelerate more quickly on slopes
			if (objectSpeed < wantedSpeed) { wantedSpeed = objectSpeed + maxAccRate; }
			if (objectSpeed > wantedSpeed) { wantedSpeed = objectSpeed - maxDecRate; }


			// note: also scale wantedSpeed by the required absolute turning angle?
			vec3f wantedDir = objectCellVel / wantedSpeed;

			{
				float forwardGlobalAngleRad = atan2f(-objectDir.z, -objectDir.x);
				float wantedGlobalAngleRad = atan2f(-wantedDir.z, -wantedDir.x);
				float deltaGlobalAngleRad = 0.0f;

				if (forwardGlobalAngleRad < 0.0f) { forwardGlobalAngleRad += (M_PI * 2.0f); }
				if (wantedGlobalAngleRad < 0.0f) { wantedGlobalAngleRad += (M_PI * 2.0f); }

				deltaGlobalAngleRad = (forwardGlobalAngleRad - wantedGlobalAngleRad);

				// take the shorter of the two possible turns
				// (positive angle means a right turn and vv)
				if (deltaGlobalAngleRad >  M_PI) { deltaGlobalAngleRad = -((M_PI * 2.0f) - deltaGlobalAngleRad); }
				if (deltaGlobalAngleRad < -M_PI) { deltaGlobalAngleRad =  ((M_PI * 2.0f) + deltaGlobalAngleRad); }

				wantedDir = objectDir.rotateY(DEG2RAD(objectDef->GetMaxTurningRate()) * ((deltaGlobalAngleRad > 0.0f)? 1.0f: -1.0f));
			}

			// note: when using the individual Set*Raw* callouts,
			// change the position last so that the object's new
			// hasMoved state is not overwritten again
			mCOH->SetSimObjectRawPhysicalState(objectID, objectPos + wantedDir * wantedSpeed, wantedDir, wantedSpeed);
		#endif
	}

	return true;
}

vec3f CCGrid::GetInterpolatedVelocity(const std::vector<Cell::Edge>& edges, const Cell* c, const vec3f& pos, const vec3f& dir) const {
	vec3f vel;

	float a = 0.0f;
	float b = 0.0f;

	#if (VELOCITY_FIELD_BILINEAR_INTERPOLATION == 1)
		// "standard" bilinear interpolation, except
		// that sample values are not stored at grid
		// corners and represent vectors rather than
		// scalars (note: the unit's direction vector
		// is not used here)
		//
		// first get the relative distance to the
		// DIR_W (a) and DIR_N (b) edges based on
		// <pos>
		a = (pos.x - c->x * mSquareSize) / mSquareSize;
		b = (pos.z - c->y * mSquareSize) / mSquareSize;

		const vec3f& vN = edges[ c->edges[DIR_N] ].velocity;
		const vec3f& vS = edges[ c->edges[DIR_S] ].velocity;
		const vec3f& vE = edges[ c->edges[DIR_E] ].velocity;
		const vec3f& vW = edges[ c->edges[DIR_W] ].velocity;

		const vec3f vTL = (vN + vW) * 0.5f; // top-left sample point
		const vec3f vTR = (vN + vE) * 0.5f; // top-right sample point
		const vec3f vBL = (vS + vW) * 0.5f; // bottom-left sample point
		const vec3f vBR = (vS + vE) * 0.5f; // bottom-right sample point

		vel += (vTL * (1.0f - a) * (1.0f - b));
		vel += (vTR * (0.0f + a) * (1.0f - b));
		vel += (vBL * (1.0f - a) * (0.0f + b));
		vel += (vBR * (0.0f + a) * (0.0f + b));
	#else
		// we know that <dir> always falls into one of four quadrants
		// therefore, we need to sample the velocity-field along two
		// of the four cardinal (NSEW) directions and interpolate
		unsigned int i = 0;
		unsigned int j = 0;

		if (dir.x >= 0.0f) {
			i = DIR_E;
			a = dir.x;
		} else {
			i = DIR_W;
			a = -dir.x;
		}

		if (dir.z >= 0.0f) {
			j = DIR_N;
			b = dir.z;
		} else {
			j = DIR_S;
			b = -dir.z;
		}

		vel += (edges[ c->edges[i] ].velocity * a);
		vel += (edges[ c->edges[j] ].velocity * b);
	#endif

	return vel;
}



// convert a world-space position to (CLAMPED) <x, y> grid-indices
vec3i CCGrid::GetCellIndex2D(const vec3f& worldPos) const {
	const int gx = (worldPos.x / mSquareSize);
	const int gz = (worldPos.z / mSquareSize);
	const int cx = CLAMP(gx, 0, int(numCellsX - 1));
	const int cz = CLAMP(gz, 0, int(numCellsZ - 1));
	return vec3i(cx, 0, cz);
}

// get the (CLAMPED ) 1D grid-cell index corresponding to a world-space position
unsigned int CCGrid::GetCellIndex1D(const vec3f& worldPos) const {
	const vec3i&       gridPos = GetCellIndex2D(worldPos);
	const unsigned int gridIdx = GRID_INDEX_UNSAFE(gridPos.x, gridPos.z);
	return gridIdx;
}

// convert a cell's <x, y> grid-indices to the
// world-space position of its top-left corner
vec3f CCGrid::GetCellCornerPos(const Cell* c) const {
	const float wx = c->x * mSquareSize;
	const float wz = c->y * mSquareSize;
	return vec3f(wx, ELEVATION(c->x, c->y), wz);
}

vec3f CCGrid::GetCellMidPos(const Cell* c) const {
	const float wx = (c->x * mSquareSize) + (mSquareSize >> 1);
	const float wz = (c->y * mSquareSize) + (mSquareSize >> 1);
	return vec3f(wx, 0.0f, wz);
}






void CCGrid::Cell::ResetFull() {
	ResetGlobalStaticVars();
	ResetGlobalDynamicVars();
	ResetGroupVars();
}

void CCGrid::Cell::ResetGlobalStaticVars() {
	height = 0.0f; staticDiscomfort = NVECf;
}

void CCGrid::Cell::ResetGlobalDynamicVars() {
	avgVelocity      = NVECf;
	mobileDiscomfort = NVECf;
	density          = 0.0f;
}

void CCGrid::Cell::ResetGroupVars() {
	potential  = std::numeric_limits<float>::infinity();

	known     = false;
	candidate = false;

	speed[DIR_N] = cost[DIR_N] = 0.0f;
	speed[DIR_S] = cost[DIR_S] = 0.0f;
	speed[DIR_E] = cost[DIR_E] = 0.0f;
	speed[DIR_W] = cost[DIR_W] = 0.0f;
}

vec3f CCGrid::Cell::GetNormalisedPotentialGradient(const std::vector<Cell::Edge>& gridEdges, unsigned int dir) const {
	const Edge* edge = &gridEdges[edges[dir]];
	const float plen = edge->potentialDelta.len2D();

	vec3f pgrad;

	if (plen > 0.0f) {
		pgrad = (edge->potentialDelta / plen);
	}

	return pgrad;
}
