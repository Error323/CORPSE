#include <cmath>
#include <algorithm>
#include <limits>

#include "./Grid.hpp"
#include "../../Ext/ICallOutHandler.hpp"
#include "../../Math/Trig.hpp"
#include "../../Sim/SimObjectDef.hpp"
#include "../../Sim/SimObjectState.hpp"
#include "../../System/Debugger.hpp"

#define EPSILON 0.01f

#define GRID_INDEX(x, y) (((y) * (numCellsX)) + (x))
#define ELEVATION(x, y) (mCOH->GetCenterHeightMap()[(mDownScale * (y)) * (mDownScale * numCellsX) + (mDownScale * (x))])

#define POSITIVE_SLOPE(dir, slope)                            \
	(((dir == DIR_N || dir == DIR_W) && (slope <  0.0f))  ||  \
	 ((dir == DIR_S || dir == DIR_E) && (slope >= 0.0f)))
#define NEGATIVE_SLOPE(dir, slope)                            \
	(((dir == DIR_N || dir == DIR_W) && (slope >= 0.0f))  ||  \
	 ((dir == DIR_S || dir == DIR_E) && (slope <  0.0f)))

#define MMAX(a, b) (((a) >= (b))? (a): (b))
#define MMIN(a, b) (((a) >= (b))? (b): (a))
#define CLAMP(v, vmin, vmax) MMAX((vmin), MMIN((vmax), (v)))

#define DENSITY_CONVERSION_TCP06                0

#define SPEED_COST_POTENTIAL_MERGED_COMPUTATION 1
#define SPEED_COST_SINGLE_PASS_COMPUTATION      0

#define VELOCITY_FIELD_DIRECT_INTERPOLATION     0
#define VELOCITY_FIELD_BILINEAR_INTERPOLATION   1



void Grid::AddGroup(unsigned int groupID) {
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
}

void Grid::DelGroup(unsigned int groupID) {
	mSpeedVisData[groupID].clear(); mSpeedVisData.erase(groupID);
	mCostVisData[groupID].clear(); mCostVisData.erase(groupID);
	mPotentialVisData[groupID].clear(); mPotentialVisData.erase(groupID);

	mVelocityVisData[groupID].clear(); mVelocityVisData.erase(groupID);
	mPotentialDeltaVisData[groupID].clear(); mPotentialDeltaVisData.erase(groupID);
}



// visualisation data accessors for scalar fields
const float* Grid::GetDensityVisDataArray() const {
	return (mDensityVisData.empty())? NULL: &mDensityVisData[0];
}

const float* Grid::GetDiscomfortVisDataArray() const {
	return (mDiscomfortVisData.empty())? NULL: &mDiscomfortVisData[0];
}

const float* Grid::GetSpeedVisDataArray(unsigned int groupID) const {
	std::map<unsigned int, std::vector<float> >::const_iterator it = mSpeedVisData.find(groupID);

	if (it == mSpeedVisData.end()) { return NULL; }
	if ((it->second).empty()) { return NULL; }

	return &(it->second)[0];
}

const float* Grid::GetCostVisDataArray(unsigned int groupID) const {
	std::map<unsigned int, std::vector<float> >::const_iterator it = mCostVisData.find(groupID);

	if (it == mCostVisData.end()) { return NULL; }
	if ((it->second).empty()) { return NULL; }

	return &(it->second)[0];
}

const float* Grid::GetHeightVisDataArray() const {
	return (mHeightVisData.empty())? NULL: &mHeightVisData[0];
}

const float* Grid::GetPotentialVisDataArray(unsigned int groupID) const {
	std::map<unsigned int, std::vector<float> >::const_iterator it = mPotentialVisData.find(groupID);

	if (it == mPotentialVisData.end()) { return NULL; }
	if ((it->second).empty()) { return NULL; }

	return &(it->second)[0];
}

// visualisation data accessors for vector fields
const vec3f* Grid::GetVelocityVisDataArray(unsigned int groupID) const {
	std::map<unsigned int, std::vector<vec3f> >::const_iterator it = mVelocityVisData.find(groupID);

	if (it == mVelocityVisData.end()) { return NULL; }
	if ((it->second).empty()) { return NULL; }

	return &(it->second)[0];
}

const vec3f* Grid::GetVelocityAvgVisDataArray() const {
	return (mAvgVelocityVisData.empty())? NULL: &mAvgVelocityVisData[0];
}

const vec3f* Grid::GetPotentialDeltaVisDataArray(unsigned int groupID) const {
	std::map<unsigned int, std::vector<vec3f> >::const_iterator it = mPotentialDeltaVisData.find(groupID);

	if (it == mPotentialDeltaVisData.end()) { return NULL; }
	if ((it->second).empty()) { return NULL; }

	return &(it->second)[0];
}

const vec3f* Grid::GetHeightDeltaVisDataArray() const {
	return (mHeightDeltaVisData.empty())? NULL: &mHeightDeltaVisData[0];
}



void Grid::Kill() {
	// clear the global scalar field visualisation data
	mDensityVisData.clear();
	mHeightVisData.clear();

	// clear the global vector field visualisation data
	mAvgVelocityVisData.clear();
	mHeightDeltaVisData.clear();

	mTouchedCells.clear();
	mBuffers[mCurrBufferIdx].cells.clear();
	mBuffers[mCurrBufferIdx].edges.clear();
	mBuffers[mPrevBufferIdx].cells.clear();
	mBuffers[mPrevBufferIdx].edges.clear();
}

void Grid::Init(unsigned int downScaleFactor, ICallOutHandler* coh) {
	PFFG_ASSERT(downScaleFactor >= 1);

	// NOTE: if mDownScale > 1, the height-map must be downsampled
	mCOH        = coh;
	mDownScale  = downScaleFactor;
	numCellsX   = mCOH->GetHeightMapSizeX() / mDownScale;
	numCellsZ   = mCOH->GetHeightMapSizeZ() / mDownScale;
	mSquareSize = mCOH->GetSquareSize()     * mDownScale;

	// NOTE:
	//   if the terrain is completely flat, these will be zero (causing DIV0's)
	//
	//   the slope (height difference) from A to B is equal to the inverse
	//   slope from B to A, therefore we take the absolute value at every
	//   cell (this means the scale term in f_topo lies in [-1, 1] rather
	//   than in [0, 1]) to determine the extrema
	mMinTerrainSlope =  std::numeric_limits<float>::max();
	mMaxTerrainSlope = -std::numeric_limits<float>::max();

	printf("[Grid::Init] resolution: %dx%d %d\n", numCellsX, numCellsZ, mSquareSize);
	printf("\tDENSITY_CONVERSION_TCP06:                %d\n", DENSITY_CONVERSION_TCP06);
	printf("\n");
	printf("\tSPEED_COST_POTENTIAL_MERGED_COMPUTATION: %d\n", SPEED_COST_POTENTIAL_MERGED_COMPUTATION);
	printf("\tSPEED_COST_SINGLE_PASS_COMPUTATION:      %d\n", SPEED_COST_SINGLE_PASS_COMPUTATION);
	printf("\n");
	printf("\tVELOCITY_FIELD_DIRECT_INTERPOLATION:     %d\n", VELOCITY_FIELD_DIRECT_INTERPOLATION);
	printf("\tVELOCITY_FIELD_BILINEAR_INTERPOLATION:   %d\n", VELOCITY_FIELD_BILINEAR_INTERPOLATION);

	const unsigned int numCells = numCellsX * numCellsZ;
	const unsigned int numEdges = (numCellsX + 1) * numCellsZ + (numCellsZ + 1) * numCellsX;

	// visualisation data for global scalar fields
	mDensityVisData.resize(numCells, 0.0f);
	mHeightVisData.resize(numCells, 0.0f);
	mDiscomfortVisData.resize(numCells, 0.0f);

	// visualisation data for global vector fields
	mAvgVelocityVisData.resize(numCells, NVECf);
	mHeightDeltaVisData.resize(numCells * NUM_DIRS);



	mBuffers[mCurrBufferIdx].cells.reserve(numCells);
	mBuffers[mCurrBufferIdx].edges.reserve(numEdges);
	mBuffers[mPrevBufferIdx].cells.reserve(numCells);
	mBuffers[mPrevBufferIdx].edges.reserve(numEdges);

	std::vector<Cell      >& currCells = mBuffers[mCurrBufferIdx].cells;
	std::vector<Cell      >& prevCells = mBuffers[mPrevBufferIdx].cells;
	std::vector<Cell::Edge>& currEdges = mBuffers[mCurrBufferIdx].edges;
	std::vector<Cell::Edge>& prevEdges = mBuffers[mPrevBufferIdx].edges;

	for (unsigned int y = 0; y < numCellsZ; y++) {
		for (unsigned int x = 0; x < numCellsX; x++) {
			currCells.push_back(Cell(x, y));
			prevCells.push_back(Cell(x, y));
			currEdges.push_back(Grid::Cell::Edge());
			prevEdges.push_back(Grid::Cell::Edge());

			Cell* currCell = &currCells.back();
			Cell* prevCell = &prevCells.back();

			unsigned int cellIdx = 0;
			unsigned int edgeIdxW = currEdges.size() - 2;
			unsigned int edgeIdxN = currEdges.size() - 1;

			currCell->edges[DIR_W] = edgeIdxW;
			currCell->edges[DIR_N] = edgeIdxN;
			prevCell->edges[DIR_W] = edgeIdxW;
			prevCell->edges[DIR_N] = edgeIdxN;

			// bind the east face of the cell west of the current cell
			if (x > 0) {
				cellIdx = GRID_INDEX(x - 1, y);
				PFFG_ASSERT(cellIdx < currCells.size());

				Cell* currWestCell = &currCells[cellIdx];
				Cell* prevWestCell = &prevCells[cellIdx];
				currWestCell->edges[DIR_E] = edgeIdxW;
				prevWestCell->edges[DIR_E] = edgeIdxW;
			}

			// bind the south face of the cell north of the current cell
			if (y > 0) {
				cellIdx = GRID_INDEX(x, y - 1);
				PFFG_ASSERT(cellIdx < currCells.size());

				Cell* currNorthCell = &currCells[cellIdx];
				Cell* prevNorthCell = &prevCells[cellIdx];
				currNorthCell->edges[DIR_S] = edgeIdxN;
				prevNorthCell->edges[DIR_S] = edgeIdxN;
			}

			// bind a new face to the southern face of the border cell
			if (y == numCellsZ - 1) {
				currEdges.push_back(Grid::Cell::Edge());
				prevEdges.push_back(Grid::Cell::Edge());
				currCell->edges[DIR_S] = currEdges.size() - 1;
				prevCell->edges[DIR_S] = prevEdges.size() - 1;
			}

			// bind a new face to the eastern face of the border cell
			if (x == numCellsX - 1) {
				currEdges.push_back(Grid::Cell::Edge());
				prevEdges.push_back(Grid::Cell::Edge());
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
			Cell* currCell = &currCells[GRID_INDEX(x, y)];
			Cell* prevCell = &prevCells[GRID_INDEX(x, y)];

			// set potential to +inf, etc.
			currCell->ResetFull();
			prevCell->ResetFull();

			// set the height, assuming the heightmap is static
			currCell->height = ELEVATION(x, y);
			prevCell->height = ELEVATION(x, y);

			// set the baseline discomfort values
			// NOTE:
			//    this assumes all groups experience discomfort in the same way,
			//    because the field is global (converting it back to per-group
			//    again would make the predictive discomfort step too expensive)
			//    therefore, maximum-slope restrictions can not be modelled via
			//    discomfort zones
			//
			//    for now, avoid higher areas (problem: discomfort is a much larger
			//    term than speed, but needs to be around same order of magnitude)
			currCell->discomfort = (currCell->height - mCOH->GetMinMapHeight()) / (mCOH->GetMaxMapHeight() - mCOH->GetMinMapHeight());
			prevCell->discomfort = (currCell->height - mCOH->GetMinMapHeight()) / (mCOH->GetMaxMapHeight() - mCOH->GetMinMapHeight());

			mHeightVisData[GRID_INDEX(x, y)] = currCell->height;
			mDiscomfortVisData[GRID_INDEX(x, y)] = currCell->discomfort;
		}
	}

	// compute gradient-heights and neighbors
	for (unsigned int y = 0; y < numCellsZ; y++) {
		for (unsigned int x = 0; x < numCellsX; x++) {
			unsigned int idx = GRID_INDEX(x, y);
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
				currNgb = &currCells[GRID_INDEX(x, y - 1)];
				prevNgb = &prevCells[GRID_INDEX(x, y - 1)];

				currEdge->heightDelta = vec3f(0.0f, 0.0f, (currNgb->height - currCell->height));
				prevEdge->heightDelta = vec3f(0.0f, 0.0f, (prevNgb->height - prevCell->height));

				currCell->neighbors[currCell->numNeighbors++] = GRID_INDEX(x, y - 1);
				prevCell->neighbors[prevCell->numNeighbors++] = GRID_INDEX(x, y - 1);

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
				currNgb = &currCells[GRID_INDEX(x, y + 1)];
				prevNgb = &prevCells[GRID_INDEX(x, y + 1)];

				currEdge->heightDelta = vec3f(0.0f, 0.0f, (currNgb->height - currCell->height));
				prevEdge->heightDelta = vec3f(0.0f, 0.0f, (prevNgb->height - prevCell->height));

				currCell->neighbors[currCell->numNeighbors++] = GRID_INDEX(x, y + 1);
				prevCell->neighbors[prevCell->numNeighbors++] = GRID_INDEX(x, y + 1);

				mMinTerrainSlope = std::min(mMinTerrainSlope, std::fabs(currEdge->heightDelta.z));
				mMaxTerrainSlope = std::max(mMaxTerrainSlope, std::fabs(currEdge->heightDelta.z));

				mHeightDeltaVisData[idx * NUM_DIRS + dir] = currEdge->heightDelta * ((mSquareSize / mDownScale) >> 1);
			}

			if (x > 0) {
				dir = DIR_W;

				currEdge = &currEdges[currCell->edges[dir]];
				prevEdge = &prevEdges[prevCell->edges[dir]];
				currNgb = &currCells[GRID_INDEX(x - 1, y)];
				prevNgb = &prevCells[GRID_INDEX(x - 1, y)];

				currEdge->heightDelta = vec3f((currNgb->height - currCell->height), 0.0f, 0.0f);
				prevEdge->heightDelta = vec3f((prevNgb->height - prevCell->height), 0.0f, 0.0f);

				currCell->neighbors[currCell->numNeighbors++] = GRID_INDEX(x - 1, y);
				prevCell->neighbors[prevCell->numNeighbors++] = GRID_INDEX(x - 1, y);

				mMinTerrainSlope = std::min(mMinTerrainSlope, std::fabs(currEdge->heightDelta.x));
				mMaxTerrainSlope = std::max(mMaxTerrainSlope, std::fabs(currEdge->heightDelta.x));

				mHeightDeltaVisData[idx * NUM_DIRS + dir] = currEdge->heightDelta * ((mSquareSize / mDownScale) >> 1);
			}

			if (x < numCellsX - 1) {
				dir = DIR_E;

				currEdge = &currEdges[currCell->edges[dir]];
				prevEdge = &prevEdges[prevCell->edges[dir]];
				currNgb = &currCells[GRID_INDEX(x + 1, y)];
				prevNgb = &prevCells[GRID_INDEX(x + 1, y)];

				currEdge->heightDelta = vec3f((currNgb->height - currCell->height), 0.0f, 0.0f);
				prevEdge->heightDelta = vec3f((prevNgb->height - prevCell->height), 0.0f, 0.0f);

				currCell->neighbors[currCell->numNeighbors++] = GRID_INDEX(x + 1, y);
				prevCell->neighbors[prevCell->numNeighbors++] = GRID_INDEX(x + 1, y);

				mMinTerrainSlope = std::min(mMinTerrainSlope, std::fabs(currEdge->heightDelta.x));
				mMaxTerrainSlope = std::max(mMaxTerrainSlope, std::fabs(currEdge->heightDelta.x));

				mHeightDeltaVisData[idx * NUM_DIRS + dir] = currEdge->heightDelta * ((mSquareSize / mDownScale) >> 1);
			}
		}
	}
}

void Grid::Reset() {
	numResets += 1;

	std::vector<Cell>& currCells = mBuffers[mCurrBufferIdx].cells;
	std::vector<Cell>& prevCells = mBuffers[mPrevBufferIdx].cells;

	// undo last frame's dynamic-global data writes
	for (std::set<unsigned int>::const_iterator it = mTouchedCells.begin(); it != mTouchedCells.end(); ++it) {
		const unsigned int idx = *it;

		Cell* currCell = &currCells[idx];
		Cell* prevCell = &prevCells[idx];

		currCell->ResetGlobalDynamicVars();
		prevCell->ResetGlobalDynamicVars();

		// restore the baseline discomfort values
		currCell->discomfort = (currCell->height - mCOH->GetMinMapHeight()) / (mCOH->GetMaxMapHeight() - mCOH->GetMinMapHeight());
		prevCell->discomfort = (currCell->height - mCOH->GetMinMapHeight()) / (mCOH->GetMaxMapHeight() - mCOH->GetMinMapHeight());

		mDensityVisData[idx]     = 0.0f;
		mDiscomfortVisData[idx]  = currCell->discomfort;
		mAvgVelocityVisData[idx] = NVECf;
	}

	mTouchedCells.clear();
}



void Grid::AddDensity(const vec3f& pos, const vec3f& vel, float radius) {
	std::vector<Cell>& currCells = mBuffers[mCurrBufferIdx].cells;
	std::vector<Cell>& prevCells = mBuffers[mPrevBufferIdx].cells;

	#if (DENSITY_CONVERSION_TCP06 == 0)
		// given the grid resolution, find the number of cells spanned by <r>
		const int cellsInRadius = (radius / (mSquareSize >> 1)) + 1;
		const Cell* cell = GetCell(WorldPosToCellID(pos), mCurrBufferIdx);

		for (int x = -cellsInRadius; x <= cellsInRadius; x++) {
			for (int z = -cellsInRadius; z <= cellsInRadius; z++) {
				const int cx = int(cell->x) + x;
				const int cz = int(cell->y) + z;

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
				// avoidance behavior around a single non-moving object?
				const float scale = 1.0f - ((std::abs(x) + std::abs(z)) / float(cellsInRadius << 1));
				const float rho = DENSITY_BAR + ((DENSITY_MIN - DENSITY_BAR - EPSILON) * scale);

				if (cx < 0 || cx >= int(numCellsX)) { continue; }
				if (cz < 0 || cz >= int(numCellsZ)) { continue; }

				Cell* cf = &currCells[ GRID_INDEX(cx, cz) ];
				Cell* cb = &prevCells[ GRID_INDEX(cx, cz) ];

				if ((x * x) + (z * z) <= (cellsInRadius * cellsInRadius)) {
					// if (vel.sqLen3D() <= EPSILON) { rho = DENSITY_MAX; }
					// if (x == 0 && z == 0) { rho = DENSITY_MAX; }

					cf->density += rho;
					cb->density += rho;
					cf->avgVelocity += (vel * rho);
					cb->avgVelocity += (vel * rho);
				}

				mTouchedCells.insert(GRID_INDEX(cx, cz));
			}
		}

	#else
		/*
		radius = radius;

		const Cell* cell = GetCell(WorldPosToCellID(pos));
		const vec3f& cellMidPos = GetCellPos(cell);

		float dx = pos.x - cellMidPos.x;
		float dy = pos.z - cellMidPos.z;
		int cx = cell->x, ncx = cell->x;
		int cy = cell->y, ncy = cell->y;

		if (dx <= 0.0f) { ncx = CLAMP(ncx - 1, 0, int(numCellsX - 1)); }
		if (dy <= 0.0f) { ncy = CLAMP(ncy - 1, 0, int(numCellsZ - 1)); }

		const Cell* cellNgb = GetCell(GRID_INDEX(ncx, ncy));
		const vec3f& cellNgbPos = GetCellPos(cellNgb);

		// normalise and clamp (to prevent excessive mMaxDensity)
		dx = CLAMP((pos.x - cellNgbPos.x) / mSquareSize, 0.1f, 0.9f);
		dy = CLAMP((pos.z - cellNgbPos.z) / mSquareSize, 0.1f, 0.9f);

		const unsigned int
			cellIdxA = GRID_INDEX(ncx, ncy),
			cellIdxB = GRID_INDEX( cx, ncy),
			cellIdxC = GRID_INDEX( cx,  cy),
			cellIdxD = GRID_INDEX(ncx,  cy);

		Cell *Af = &currCells[cellIdxA], *Ab = &prevCells[cellIdxA]; mTouchedCells.insert(cellIdxA);
		Cell *Bf = &currCells[cellIdxB], *Bb = &prevCells[cellIdxB]; mTouchedCells.insert(cellIdxB);
		Cell *Cf = &currCells[cellIdxC], *Cb = &prevCells[cellIdxC]; mTouchedCells.insert(cellIdxC);
		Cell *Df = &currCells[cellIdxD], *Db = &prevCells[cellIdxD]; mTouchedCells.insert(cellIdxD);

		// lambda derivation:
		//     rho_min (constant)      >=      rho_bar (constant)
		//     rho_bar                  =      1 / (2 ** lambda)
		//     rho_bar * (2 ** lambda)  =      1
		//               (2 ** lambda)  =      1 / rho_bar
		//            log(2 ** lambda)  =  log(1 / rho_bar)
		//           (lambda * log(2))  = (log(1) - log(rho_bar))
		//                      lambda  = (-log(rho_bar) / log(2))
		//
		// this is a positive number if and only if 0.0 < rho_bar <= 1.0,
		// so we "expect" rho_min and rho_max to lie in the range [0.0, 1.0]
		//
		//    if they do, then dx and dy will only be raised to exponents in
		//    [+inf, +0.0], whereas if they do not, then dx and dy will be
		//    raised to exponents in [+inf, -inf] and it is much harder to
		//    define "reasonable" thresholds for "low" and "high" density
		//
		//    however, in BOTH cases, cell densities are *not* guaranteed to
		//    lie in [0.0, 1.0] (even when dx and dy themselves do), so they
		//    still need normalisation (if rho_* is normalised) with just one
		//    exception: d{x,y} in [0.0, 1.0] and lambda in [0.0, 1.0]
		//
		// since we want to achieve radial density falloff, larger values for
		// dx or dy must result in smaller densities *regardless* of lambda:
		//    positive non-fractional dx and dy,  positive non-fractional lambda  ==>   2.00^ 5.0 = 32.00000,  4.00^ 5.0 = 1024.000 (WRONG: no falloff)
		//    positive non-fractional dx and dy,  positive     fractional lambda  ==>   2.00^ 0.5 =  1.41421,  4.00^ 0.5 =    2.000 (WRONG: no falloff)
		//    positive non-fractional dx and dy,  negative non-fractional lambda  ==>   2.00^-5.0 =  0.03125,  4.00^-5.0 =    0.001
		//    positive non-fractional dx and dy,  negative     fractional lambda  ==>   2.00^-0.5 =  0.70710,  4.00^-0.5 =    0.500
		//    ...
		//    positive     fractional dx and dy,  positive non-fractional lambda  ==>   0.50^ 5.0 =  0.03125,  0.75^ 5.0 =    0.237 (WRONG: no falloff)
		//    positive     fractional dx and dy,  positive     fractional lambda  ==>   0.50^ 0.5 =  0.70710,  0.75^ 0.5 =    0.866 (WRONG: no falloff)
		//    positive     fractional dx and dy,  negative non-fractional lambda  ==>   0.50^-5.0 = 32.00000,  0.75^-5.0 =    4.213
		//    positive     fractional dx and dy,  negative     fractional lambda  ==>   0.50^-0.5 =  1.41421,  0.75^-0.5 =    1.154
		//
		// take the *non-normalised* DENSITY_BAR value, so that lambda is
		// always a negative number (fractional if [1.0/DENSITY_BAR] < 2.0,
		// fractional otherwise)
		static const float DENSITY_EXP = -(logf(1.0f / DENSITY_BAR) / logf(2.0f));

		// splat the density
		const float rhoA = powf(std::min<float>(1.0f - dx, 1.0f - dy), DENSITY_EXP);
		const float rhoB = powf(std::min<float>(       dx, 1.0f - dy), DENSITY_EXP);
		const float rhoC = powf(std::min<float>(       dx,        dy), DENSITY_EXP);
		const float rhoD = powf(std::min<float>(1.0f - dx,        dy), DENSITY_EXP);

		Af->density += rhoA; Ab->density = Af->density;
		Bf->density += rhoB; Bb->density = Bf->density;
		Cf->density += rhoC; Cb->density = Cf->density;
		Df->density += rhoD; Db->density = Df->density;

		// add velocity (NOTE: should only C receive this?)
		Af->avgVelocity += (vel * rhoA); Bf->avgVelocity += (vel * rhoB); Cf->avgVelocity += (vel * rhoC); Df->avgVelocity += (vel * rhoD);
		Ab->avgVelocity += (vel * rhoA); Bb->avgVelocity += (vel * rhoB); Cb->avgVelocity += (vel * rhoC); Db->avgVelocity += (vel * rhoD);
		*/
	#endif
}

void Grid::AddDiscomfort(const vec3f& pos, const vec3f& vel, float radius, unsigned int numFrames, float stepSize) {
	if (vel.sqLen3D() <= EPSILON) {
		return;
	}

	std::vector<Cell>& currCells = mBuffers[mCurrBufferIdx].cells;
	std::vector<Cell>& prevCells = mBuffers[mPrevBufferIdx].cells;

	const int cellsInRadius = (radius / (mSquareSize >> 1)) + 1;
	const unsigned int posCellIdx = WorldPosToCellID(pos);

	for (unsigned int n = 0; n < numFrames; n++) {
		const vec3f        cellPos = pos + (vel * n * stepSize);
		const unsigned int cellIdx = WorldPosToCellID(cellPos);

		// skip our own cell
		if (cellIdx != posCellIdx) {
			Cell* currCell = &currCells[cellIdx];
			Cell* prevCell = &prevCells[cellIdx];

			#if (DENSITY_CONVERSION_TCP06 == 1)
				currCell->discomfort += DENSITY_BAR;
				prevCell->discomfort += DENSITY_BAR;

				mTouchedCells.insert(cellIdx);
			#else
				prevCell = prevCell;

				for (int x = -cellsInRadius; x <= cellsInRadius; x++) {
					for (int z = -cellsInRadius; z <= cellsInRadius; z++) {
						const int cx = int(currCell->x) + x;
						const int cz = int(currCell->y) + z;

						if (cx < 0 || cx >= int(numCellsX)) { continue; }
						if (cz < 0 || cz >= int(numCellsZ)) { continue; }

						Cell* cf = &currCells[ GRID_INDEX(cx, cz) ];
						Cell* cb = &prevCells[ GRID_INDEX(cx, cz) ];

						if ((x * x) + (z * z) <= (cellsInRadius * cellsInRadius)) {
							// note: use more plausible values?
							cf->discomfort += DENSITY_BAR;
							cb->discomfort += DENSITY_BAR;

							mTouchedCells.insert(GRID_INDEX(cx, cz));
						}
					}
				}
			#endif
		}
	}
}

void Grid::ComputeAvgVelocity() {
	std::vector<Cell>& currCells = mBuffers[mCurrBufferIdx].cells;
	std::vector<Cell>& prevCells = mBuffers[mPrevBufferIdx].cells;

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

		// note: unnecessary?
		cf->density = CLAMP(cf->density, 0.0f, DENSITY_MAX + EPSILON);
		cb->density = CLAMP(cb->density, 0.0f, DENSITY_MAX + EPSILON);

		// normalise the densities, so comparisons with
		// DENSITY_MIN and DENSITY_MAX are well-defined
		// when constructing the speed-field ==> already
		// guaranteed regardless of the rho_* scale
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
		mDiscomfortVisData[idx]  = cf->discomfort;
		mAvgVelocityVisData[idx] = cf->avgVelocity;
	}
}






#if (SPEED_COST_POTENTIAL_MERGED_COMPUTATION == 0)
	void Grid::ComputeCellSpeed(unsigned int groupID, unsigned int cellIdx, std::vector<Cell>& currCells, std::vector<Cell::Edge>& currEdges) {
		Cell* currCell = &currCells[cellIdx];

		const vec3f& cellPos = GetCellPos(currCell);

		for (unsigned int dir = 0; dir < NUM_DIRS; dir++) {
			// this assumes a unit's contribution to the density field
			// is no greater than rho-bar outside a disc of radius <r>
			// (such that f == f_topological when rho_min >= rho_bar)
			// also note that if mMaxGroupRadius < (mSquareSize >> 1)
			// this just maps to <currCell>
			const vec3f densityDirOffset = mDirVectors[dir] * (mMaxGroupRadius + EPSILON);

			const Cell::Edge* currCellDirEdge = &currEdges[currCell->edges[dir]];
			const Cell*       currCellDirNgb  = GetCell(WorldPosToCellID(cellPos + densityDirOffset), mCurrBufferIdx);

			const float cellDirSlope    = currCellDirEdge->heightDelta.dot2D(mDirVectors[dir]);
			      float cellDirSlopeMod = 0.0f;

			// f_{M --> dir} for computing f, based on offset density
			float cellDirSpeed = 0.0f;

			{
				if (POSITIVE_SLOPE(dir, cellDirSlope)) { cellDirSlopeMod =  std::fabs(cellDirSlope); }
				if (NEGATIVE_SLOPE(dir, cellDirSlope)) { cellDirSlopeMod = -std::fabs(cellDirSlope); }

				const float densityDirSpeedScale = (currCellDirNgb->density - DENSITY_MIN) / (DENSITY_MAX - DENSITY_MIN);
				const float slopeDirSpeedScale   = (cellDirSlopeMod - mMinTerrainSlope) / (mMaxTerrainSlope - mMinTerrainSlope);
				const float cellDirTopoSpeed     = mMaxGroupSpeed + CLAMP(slopeDirSpeedScale, -1.0f, 1.0f) * (mMinGroupSpeed - mMaxGroupSpeed);
				const float cellDirFlowSpeed     = std::max(0.0f, currCellDirNgb->avgVelocity.dot2D(mDirVectors[dir]));
				const float cellDirTopoFlowSpeed = cellDirTopoSpeed + densityDirSpeedScale * (cellDirTopoSpeed - cellDirFlowSpeed);

				cellDirSpeed = cellDirTopoFlowSpeed;

				if (currCellDirNgb->density >= DENSITY_MAX) { cellDirSpeed = cellDirFlowSpeed; }
				if (currCellDirNgb->density <= DENSITY_MIN) { cellDirSpeed = cellDirTopoSpeed; }
			}

			currCell->speed[dir] = cellDirSpeed;
			mSpeedVisData[groupID][cellIdx * NUM_DIRS + dir] = cellDirSpeed;
		}
	}

	void Grid::ComputeCellCost(unsigned int groupID, unsigned int cellIdx, std::vector<Cell>& currCells, std::vector<Cell::Edge>& currEdges) {
		Cell* currCell = &currCells[cellIdx];

		for (unsigned int dir = 0; dir < NUM_DIRS; dir++) {
			const Cell::Edge* currCellDirEdge = &currEdges[currCell->edges[dir]];
			const Cell*       currCellDirNgb  = NULL;

			switch (dir) {
				case DIR_N: { currCellDirNgb = (currCell->y >             0)? GetCell(GRID_INDEX(currCell->x,     currCell->y - 1), mCurrBufferIdx): currCell; } break;
				case DIR_S: { currCellDirNgb = (currCell->y < numCellsZ - 1)? GetCell(GRID_INDEX(currCell->x,     currCell->y + 1), mCurrBufferIdx): currCell; } break;
				case DIR_E: { currCellDirNgb = (currCell->x < numCellsX - 1)? GetCell(GRID_INDEX(currCell->x + 1, currCell->y    ), mCurrBufferIdx): currCell; } break;
				case DIR_W: { currCellDirNgb = (currCell->x >             0)? GetCell(GRID_INDEX(currCell->x - 1, currCell->y    ), mCurrBufferIdx): currCell; } break;
			}

			const float cellDirDiscomfort = (currCellDirNgb->height - mCOH->GetMinMapHeight()) / (mCOH->GetMaxMapHeight() - mCOH->GetMinMapHeight());
			const float cellDirSlope      = currCellDirEdge->heightDelta.dot2D(mDirVectors[dir]);
			      float cellDirSlopeMod   = 0.0f;

			float cellDirSpeed = 0.0f; // f_{M --> dir} for computing C, based on direct neighbor density
			float cellDirCost  = 0.0f; // C_{M --> dir}

			{
				if (POSITIVE_SLOPE(dir, cellDirSlope)) { cellDirSlopeMod =  std::fabs(cellDirSlope); }
				if (NEGATIVE_SLOPE(dir, cellDirSlope)) { cellDirSlopeMod = -std::fabs(cellDirSlope); }

				const float densityDirSpeedScale = (currCellDirNgb->density - DENSITY_MIN) / (DENSITY_MAX - DENSITY_MIN);
				const float slopeDirSpeedScale   = (cellDirSlopeMod - mMinTerrainSlope) / (mMaxTerrainSlope - mMinTerrainSlope);
				const float cellDirTopoSpeed     = mMaxGroupSpeed + CLAMP(slopeDirSpeedScale, -1.0f, 1.0f) * (mMinGroupSpeed - mMaxGroupSpeed);
				const float cellDirFlowSpeed     = std::max(0.0f, currCellDirNgb->avgVelocity.dot2D(mDirVectors[dir]));
				const float cellDirTopoFlowSpeed = cellDirTopoSpeed + densityDirSpeedScale * (cellDirTopoSpeed - cellDirFlowSpeed);

				cellDirSpeed = cellDirTopoFlowSpeed;

				if (currCellDirNgb->density >= DENSITY_MAX) { cellDirSpeed = cellDirFlowSpeed; }
				if (currCellDirNgb->density <= DENSITY_MIN) { cellDirSpeed = cellDirTopoSpeed; }

				if (cellDirSpeed > EPSILON) {
					cellDirCost = ((SPEED_WEIGHT * cellDirSpeed) + (DISCOMFORT_WEIGHT * cellDirDiscomfort)) / cellDirSpeed;
				} else {
					cellDirSpeed = EPSILON;
					cellDirCost = ((SPEED_WEIGHT * cellDirSpeed) + (DISCOMFORT_WEIGHT * cellDirDiscomfort)) / cellDirSpeed;
				}
			}

			currCell->cost[dir] = cellDirCost;
			mCostVisData[groupID][cellIdx * NUM_DIRS + dir] = cellDirCost;
		}
	}
	// ComputeCellSpeedAndCost(uint, uint) falls within the
	// #if branch as well, but is re-used when MERGED == 1
	// so we always compile it
#endif

// compute the speed- and unit-cost fields, per cell
//
// NOTE: engine slope-representation should be the same?
// NOTE:
//    if the flow-speed is zero in a region, then the cost
//    for cells with normalised density >= DENSITY_MAX will
//    be infinite everywhere and the potential-field update
//    can trigger asserts; this will also happen for cells
//    with density <= DENSITY_MIN whenever topoSpeed is less
//    than or equal to zero
// FIXME:
//    at coarse grid resolutions, the density offset of the
//    neighbor cell often just maps back to the current one,
//    (units take mSquareSize / mMaxGroupSpeed sim-frames to
//    traverse one cell horizontally or vertically) so that
//    flow-speed becomes zero due to self-density influence
//
void Grid::ComputeCellSpeedAndCost(unsigned int groupID, unsigned int cellIdx, std::vector<Cell>& currCells, std::vector<Cell::Edge>& currEdges) {
	Cell* currCell = &currCells[cellIdx];

	const vec3f& cellPos = GetCellPos(currCell);

	for (unsigned int dir = 0; dir < NUM_DIRS; dir++) {
		const vec3f densityDirOffset = mDirVectors[dir] * (mMaxGroupRadius + EPSILON);

		const Cell::Edge* currCellDirEdge = &currEdges[currCell->edges[dir]];
		const Cell*       currCellDirNgbR = GetCell(WorldPosToCellID(cellPos + densityDirOffset), mCurrBufferIdx);
		const Cell*       currCellDirNgbC = NULL;

		switch (dir) {
			case DIR_N: { currCellDirNgbC = (currCell->y >             0)? GetCell(GRID_INDEX(currCell->x,     currCell->y - 1), mCurrBufferIdx): currCell; } break;
			case DIR_S: { currCellDirNgbC = (currCell->y < numCellsZ - 1)? GetCell(GRID_INDEX(currCell->x,     currCell->y + 1), mCurrBufferIdx): currCell; } break;
			case DIR_E: { currCellDirNgbC = (currCell->x < numCellsX - 1)? GetCell(GRID_INDEX(currCell->x + 1, currCell->y    ), mCurrBufferIdx): currCell; } break;
			case DIR_W: { currCellDirNgbC = (currCell->x >             0)? GetCell(GRID_INDEX(currCell->x - 1, currCell->y    ), mCurrBufferIdx): currCell; } break;
		}

		const float cellDirDiscomfort = (currCellDirNgbC->height - mCOH->GetMinMapHeight()) / (mCOH->GetMaxMapHeight() - mCOH->GetMinMapHeight());
		const float cellDirSlope      = currCellDirEdge->heightDelta.dot2D(mDirVectors[dir]);
		      float cellDirSlopeMod   = 0.0f;

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

			const float densityDirSpeedScaleR = (currCellDirNgbR->density - DENSITY_MIN) / (DENSITY_MAX - DENSITY_MIN);
			const float densityDirSpeedScaleC = (currCellDirNgbC->density - DENSITY_MIN) / (DENSITY_MAX - DENSITY_MIN);

			const float slopeDirSpeedScale   = (cellDirSlopeMod - mMinTerrainSlope) / (mMaxTerrainSlope - mMinTerrainSlope);
			const float cellDirTopoSpeed     = mMaxGroupSpeed + CLAMP(slopeDirSpeedScale, -1.0f, 1.0f) * (mMinGroupSpeed - mMaxGroupSpeed);

			const float cellDirFlowSpeedR = std::max(0.0f, currCellDirNgbR->avgVelocity.dot2D(mDirVectors[dir]));
			const float cellDirFlowSpeedC = std::max(0.0f, currCellDirNgbC->avgVelocity.dot2D(mDirVectors[dir]));

			const float cellDirTopoFlowSpeedR = cellDirTopoSpeed + densityDirSpeedScaleR * (cellDirTopoSpeed - cellDirFlowSpeedR);
			const float cellDirTopoFlowSpeedC = cellDirTopoSpeed + densityDirSpeedScaleC * (cellDirTopoSpeed - cellDirFlowSpeedC);

			// default to linear interpolation of topological and flow speed
			cellDirSpeedR = cellDirTopoFlowSpeedR;
			cellDirSpeedC = cellDirTopoFlowSpeedC;

			if (currCellDirNgbR->density >= DENSITY_MAX) { cellDirSpeedR = cellDirFlowSpeedR; }
			if (currCellDirNgbR->density <= DENSITY_MIN) { cellDirSpeedR = cellDirTopoSpeed;  }

			if (currCellDirNgbC->density >= DENSITY_MAX) { cellDirSpeedC = cellDirFlowSpeedC; }
			if (currCellDirNgbC->density <= DENSITY_MIN) { cellDirSpeedC = cellDirTopoSpeed;  }

			if (cellDirSpeedC > EPSILON) {
				cellDirCost = ((SPEED_WEIGHT * cellDirSpeedC) + (DISCOMFORT_WEIGHT * cellDirDiscomfort)) / cellDirSpeedC;
			} else {
				// should this case be allowed to happen?
				// (infinite costs very heavily influence
				// behavior of potential-field generation)
				//
				// cost = std::numeric_limits<float>::infinity();
				// cost = std::numeric_limits<float>::max();
				cellDirSpeedC = EPSILON;
				cellDirCost = ((SPEED_WEIGHT * cellDirSpeedC) + (DISCOMFORT_WEIGHT * cellDirDiscomfort)) / cellDirSpeedC;
			}
		}

		currCell->speed[dir] = cellDirSpeedR;
		currCell->cost[dir] = cellDirCost;

		mSpeedVisData[groupID][cellIdx * NUM_DIRS + dir] = cellDirSpeedR;
		mCostVisData[groupID][cellIdx * NUM_DIRS + dir] = cellDirCost;
	}
}



#if (SPEED_COST_POTENTIAL_MERGED_COMPUTATION == 1)
	void Grid::ComputeCellSpeedAndCostMERGED(unsigned int groupID, Cell* currCell, std::vector<Cell>& currCells, std::vector<Cell::Edge>& currEdges) {
		// recycled from (MERGED == 0 && SINGLE_PASS == 1)
		ComputeCellSpeedAndCost(groupID, GRID_INDEX(currCell->x, currCell->y), currCells, currEdges);
	}
#else
	void Grid::ComputeSpeedAndCost(unsigned int groupID) {
		std::vector<Cell      >& currCells = mBuffers[mCurrBufferIdx].cells;
		std::vector<Cell::Edge>& currEdges = mBuffers[mCurrBufferIdx].edges;

		#if (SPEED_COST_SINGLE_PASS_COMPUTATION == 1)
			for (unsigned int cellIdx = 0; cellIdx < (numCellsX * numCellsZ); cellIdx++) {
				ComputeCellSpeedAndCost(groupID, cellIdx, currCells, currEdges);
			}
		#else
			for (unsigned int cellIdx = 0; cellIdx < (numCellsX * numCellsZ); cellIdx++) { ComputeCellSpeed(groupID, cellIdx, currCells, currEdges); }
			for (unsigned int cellIdx = 0; cellIdx < (numCellsX * numCellsZ); cellIdx++) { ComputeCellCost(groupID, cellIdx, currCells, currEdges); }
		#endif
	}
#endif






void Grid::UpdateGroupPotentialField(unsigned int groupID, const std::set<unsigned int>& goalIDs, const std::set<unsigned int>& objectIDs) {
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

	std::vector<float>& potVisData      = mPotentialVisData[groupID];
	std::vector<vec3f>& velVisData      = mVelocityVisData[groupID];
	std::vector<vec3f>& potDeltaVisData = mPotentialDeltaVisData[groupID];

	std::vector<Cell      >& currCells = mBuffers[mCurrBufferIdx].cells;
	std::vector<Cell      >& prevCells = mBuffers[mPrevBufferIdx].cells;
	std::vector<Cell::Edge>& currEdges = mBuffers[mCurrBufferIdx].edges;
	std::vector<Cell::Edge>& prevEdges = mBuffers[mPrevBufferIdx].edges;

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
		ComputeCellSpeedAndCostMERGED(groupID, currCell, currCells, currEdges);
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

		cellIdx = GRID_INDEX(currCell->x, currCell->y);

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

void Grid::UpdateCandidates(unsigned int groupID, const Cell* parent) {
	std::vector<Cell      >& currCells = mBuffers[mCurrBufferIdx].cells;
	std::vector<Cell      >& prevCells = mBuffers[mPrevBufferIdx].cells;
	std::vector<Cell::Edge>& currEdges = mBuffers[mCurrBufferIdx].edges;
	std::vector<Cell::Edge>& prevEdges = mBuffers[mPrevBufferIdx].edges;

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

		dirCells[DIR_N] = (currNgb->y >             0) ? &currCells[GRID_INDEX(currNgb->x    , currNgb->y - 1)] : NULL;
		dirCells[DIR_S] = (currNgb->y < numCellsZ - 1) ? &currCells[GRID_INDEX(currNgb->x    , currNgb->y + 1)] : NULL;
		dirCells[DIR_E] = (currNgb->x < numCellsX - 1) ? &currCells[GRID_INDEX(currNgb->x + 1, currNgb->y    )] : NULL;
		dirCells[DIR_W] = (currNgb->x >             0) ? &currCells[GRID_INDEX(currNgb->x - 1, currNgb->y    )] : NULL;

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



float Grid::Potential1D(const float p, const float c) const {
	return std::max<float>(p + c, p - c);
}

float Grid::Potential2D(const float p1, const float c1, const float p2, const float c2) const {
	// (c1^2*p2 + c2^2*p1) / (c1^2+c2^2) +/- (c1*c2 / sqrt(c1^2 + c2^2))
	const float c1s = c1 * c1;
	const float c2s = c2 * c2;
	const float c1s_plus_c2s = c1s + c2s;

	PFFG_ASSERT(c1s_plus_c2s > 0.0f);

	const float a = (c1s * p2 + c2s * p1) / (c1s_plus_c2s);
	const float b = sqrtf(c1s_plus_c2s);
	const float c = c1 * c2 / b;

	return std::max<float>(a + c, a - c);
}



bool Grid::UpdateSimObjectLocation(unsigned int objectID, unsigned int objectCellID) {
	const SimObjectDef* objectDef = mCOH->GetSimObjectDef(objectID);

	const vec3f& objectPos = mCOH->GetSimObjectPosition(objectID);
	const vec3f& objectDir = mCOH->GetSimObjectDirection(objectID);

	const std::vector<Cell      >& currCells = mBuffers[mCurrBufferIdx].cells;
	const std::vector<Cell::Edge>& currEdges = mBuffers[mCurrBufferIdx].edges;

	const Cell* objectCell = &currCells[objectCellID];
	const vec3f& objectCellVel = GetInterpolatedVelocity(currEdges, objectCell, objectPos, objectDir);

	PFFG_ASSERT_MSG(!(std::isnan(objectCellVel.x) || std::isnan(objectCellVel.y) || std::isnan(objectCellVel.z)), "Inf velocity-field for cell %u", cellIdx);
	PFFG_ASSERT_MSG(!(std::isinf(objectCellVel.x) || std::isinf(objectCellVel.y) || std::isinf(objectCellVel.z)), "NaN velocity-field for cell %u", cellIdx);

	if (objectCellVel.sqLen3D() > EPSILON) {
		#if (VELOCITY_FIELD_DIRECT_INTERPOLATION == 1)
			mCOH->SetSimObjectRawPhysicalState(objectID, objectPos + objectCellVel, objectCellVel.norm(), objectCellVel.len3D());
		#else
			const float objectSpeed = mCOH->GetSimObjectSpeed(objectID);
			const float maxAccRate = objectDef->GetMaxAccelerationRate();
			const float maxDecRate = objectDef->GetMaxDeccelerationRate();

			// in theory, the velocity-field should never cause units
			// in any group to exceed that group's speed limitations
			// (note that this is not true on slopes)
			// float wantedSpeed = std::min(objectCellVel.len3D(), objectDef->GetMaxForwardSpeed());
			float wantedSpeed = objectCellVel.len3D();

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

vec3f Grid::GetInterpolatedVelocity(const std::vector<Cell::Edge>& edges, const Cell* c, const vec3f& pos, const vec3f& dir) const {
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
		// FIXME: the interpolated vector can be near
		// <0, 0, 0>, which causes units to get stuck
		// on the grid (happens frequently at coarser
		// resolutions, related to density projection)
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



// convert a world-space position to <x, y> grid-indices
vec3i Grid::WorldPosToGridIdx(const vec3f& worldPos) const {
	const int gx = (worldPos.x / mSquareSize);
	const int gz = (worldPos.z / mSquareSize);
	const int cx = CLAMP(gx, 0, int(numCellsX - 1));
	const int cz = CLAMP(gz, 0, int(numCellsZ - 1));
	return vec3i(cx, 0, cz);
}

// get the (clamped) 1D grid-cell index corresponding to a world-space position
unsigned int Grid::WorldPosToCellID(const vec3f& worldPos) const {
	const vec3i&       gridPos = WorldPosToGridIdx(worldPos);
	const unsigned int gridIdx = GRID_INDEX(gridPos.x, gridPos.z);

	PFFG_ASSERT_MSG(gridIdx < mBuffers[0].cells.size(), "world(%2.2f, %2.2f) grid(%d, %d)", worldPos.x, worldPos.z, gridPos.x, gridPos.z);

	return gridIdx;
}

// convert a cell's <x, y> grid-indices to world-space coordinates
vec3f Grid::GridIdxToWorldPos(const Cell* c) const {
	const float wx = c->x * mSquareSize;
	const float wz = c->y * mSquareSize;
	return vec3f(wx, ELEVATION(c->x, c->y), wz);
}






void Grid::Cell::ResetFull() {
	ResetGlobalStaticVars();
	ResetGlobalDynamicVars();
	ResetGroupVars();
}

void Grid::Cell::ResetGlobalStaticVars() {
	height = 0.0f;
}

void Grid::Cell::ResetGlobalDynamicVars() {
	avgVelocity = NVECf;
	density     = 0.0f;
	discomfort  = 0.0f;
}

void Grid::Cell::ResetGroupVars() {
	potential  = std::numeric_limits<float>::infinity();

	known     = false;
	candidate = false;

	speed[DIR_N] = cost[DIR_N] = 0.0f;
	speed[DIR_S] = cost[DIR_S] = 0.0f;
	speed[DIR_E] = cost[DIR_E] = 0.0f;
	speed[DIR_W] = cost[DIR_W] = 0.0f;
}

vec3f Grid::Cell::GetNormalisedPotentialGradient(const std::vector<Cell::Edge>& gridEdges, unsigned int dir) const {
	const Edge* edge = &gridEdges[edges[dir]];
	const vec3f& edgePotDelta = edge->potentialDelta;
	const float edgePotDeltaLen = edgePotDelta.len2D();

	if (edgePotDeltaLen > 0.0f) {
		return (edgePotDelta / edgePotDeltaLen);
	}

	return NVECf;
}
