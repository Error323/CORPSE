#include <cmath>
#include <algorithm>
#include <limits>

#include "./Grid.hpp"
#include "../../Ext/ICallOutHandler.hpp"
#include "../../Math/Trig.hpp"
#include "../../Sim/SimObjectDef.hpp"
#include "../../Sim/SimObjectState.hpp"
#include "../../System/Debugger.hpp"

#define GRID_INDEX(x, y) (((y) * (mWidth)) + (x))
#define ELEVATION(x, y) (mCOH->GetCenterHeightMap()[(mDownScale * (y)) * (mDownScale * mWidth) + (mDownScale * (x))])
#define CLAMP(f, fmin, fmax) std::max(fmin, std::min(fmax, (f)))

void Grid::AddGroup(unsigned int groupID) {
	mDiscomfortVisData[groupID] = std::vector<float>();
	mDiscomfortVisData[groupID].resize(mWidth * mHeight, 0.0f);
	mSpeedVisData[groupID] = std::vector<float>();
	mSpeedVisData[groupID].resize(mWidth * mHeight * NUM_DIRS, 0.0f);
	mCostVisData[groupID] = std::vector<float>();
	mCostVisData[groupID].resize(mWidth * mHeight * NUM_DIRS, 0.0f);

	mPotentialVisData[groupID] = std::vector<float>();
	mPotentialVisData[groupID].resize(mWidth * mHeight, 0.0f);

	mVelocityVisData[groupID] = std::vector<vec3f>();
	mVelocityVisData[groupID].resize(mWidth * mHeight * NUM_DIRS, NVECf);

	mPotentialDeltaVisData[groupID] = std::vector<vec3f>();
	mPotentialDeltaVisData[groupID].resize(mWidth * mHeight * NUM_DIRS, NVECf);
}

void Grid::DelGroup(unsigned int groupID) {
	mDiscomfortVisData[groupID].clear(); mDiscomfortVisData.erase(groupID);
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

const float* Grid::GetDiscomfortVisDataArray(unsigned int groupID) const {
	std::map<unsigned int, std::vector<float> >::const_iterator it = mDiscomfortVisData.find(groupID);

	if (it == mDiscomfortVisData.end()) { return NULL; }
	if ((it->second).empty()) { return NULL; }

	return &(it->second)[0];
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
	mInitCells.clear();
	mInitEdges.clear();
	mBuffers[mFrontBufferIdx].cells.clear();
	mBuffers[mFrontBufferIdx].edges.clear();
	mBuffers[mBackBufferIdx].cells.clear();
	mBuffers[mBackBufferIdx].edges.clear();
}

void Grid::Init(unsigned int downScaleFactor, ICallOutHandler* coh) {
	PFFG_ASSERT(downScaleFactor >= 1);

	// NOTE: if mDownScale > 1, the engine's height-map must be downsampled
	mCOH        = coh;
	mDownScale  = downScaleFactor;
	mWidth      = mCOH->GetHeightMapSizeX() / mDownScale;
	mHeight     = mCOH->GetHeightMapSizeZ() / mDownScale;
	mSquareSize = mCOH->GetSquareSize()     * mDownScale;

	// NOTE:
	//   if the terrain is completely flat, these will be zero (causing DIV0's)
	//
	//   the slope (height difference) from A to B is equal to the inverse
	//   slope from B to A, therefore we take the absolute value at every
	//   cell (this means the scale term in f_topo lies in [-1, 1] rather
	//   than in [0, 1]) for determining the extrema
	mMinTerrainSlope =  std::numeric_limits<float>::max();
	mMaxTerrainSlope = -std::numeric_limits<float>::max();

	printf("[Grid::Init] resolution: %dx%d %d\n", mWidth, mHeight, mSquareSize);


	const unsigned int numCells = mWidth * mHeight;
	const unsigned int numEdges = (mWidth + 1) * mHeight + (mHeight + 1) * mWidth;

	// visualisation data for global scalar fields
	mDensityVisData.resize(numCells);
	mHeightVisData.resize(numCells);

	// visualisation data for global vector fields
	mAvgVelocityVisData.resize(numCells);
	mHeightDeltaVisData.resize(numCells * NUM_DIRS);


	mInitCells.reserve(numCells);
	mInitEdges.reserve(numEdges);

	mBuffers[mFrontBufferIdx].cells.reserve(numCells);
	mBuffers[mFrontBufferIdx].edges.reserve(numEdges);
	mBuffers[mBackBufferIdx].cells.reserve(numCells);
	mBuffers[mBackBufferIdx].edges.reserve(numEdges);

	for (unsigned int y = 0; y < mHeight; y++) {
		for (unsigned int x = 0; x < mWidth; x++) {
			mInitCells.push_back(Cell(x, y));
			mInitEdges.push_back(Grid::Cell::Edge());
			mInitEdges.push_back(Grid::Cell::Edge());

			Cell* curCell = &mInitCells.back();

			unsigned int cellIdx = 0;
			unsigned int edgeIdxW = mInitEdges.size() - 2;
			unsigned int edgeIdxN = mInitEdges.size() - 1;

			curCell->edges[DIR_W]  = edgeIdxW;
			curCell->edges[DIR_N] = edgeIdxN;

			// bind the east face of the cell west of the current cell
			if (x > 0) {
				cellIdx = GRID_INDEX(x - 1, y);
				PFFG_ASSERT(cellIdx < mInitCells.size());

				Cell* westCell = &mInitCells[cellIdx];
				westCell->edges[DIR_E] = edgeIdxW;
			}

			// bind the south face of the cell north of the current cell
			if (y > 0) {
				cellIdx = GRID_INDEX(x, y - 1);
				PFFG_ASSERT(cellIdx < mInitCells.size());

				Cell* northCell = &mInitCells[cellIdx];
				northCell->edges[DIR_S] = edgeIdxN;
			}

			// bind a new face to the southern face of the border cell
			if (y == mHeight - 1) {
				mInitEdges.push_back(Grid::Cell::Edge());
				curCell->edges[DIR_S] = mInitEdges.size() - 1;
			}

			// bind a new face to the eastern face of the border cell
			if (x == mWidth - 1) {
				mInitEdges.push_back(Grid::Cell::Edge());
				curCell->edges[DIR_E] = mInitEdges.size() - 1;
			}
		}
	}

	PFFG_ASSERT(mInitCells.size() == numCells);
	PFFG_ASSERT(mInitEdges.size() == numEdges);

	// perform a full reset of the cells and compute their heights
	for (unsigned int y = 0; y < mHeight; y++) {
		for (unsigned int x = 0; x < mWidth; x++) {
			Cell* curCell = &mInitCells[GRID_INDEX(x, y)];

			// set potential to +inf, etc.
			curCell->ResetFull();
			// set the height, assuming the heightmap is static
			curCell->height = ELEVATION(x, y);

			mHeightVisData[GRID_INDEX(x, y)] = curCell->height;
		}
	}

	// compute gradient-heights and neighbors
	for (unsigned int y = 0; y < mHeight; y++) {
		for (unsigned int x = 0; x < mWidth; x++) {
			unsigned int idx = GRID_INDEX(x, y);
			unsigned int dir = 0;

			Cell* cell = &mInitCells[idx];
			Cell* ngb = NULL;
			Cell::Edge* edge = NULL;

			if (y > 0) {
				dir = DIR_N;

				edge = &mInitEdges[cell->edges[dir]];
				ngb = &mInitCells[GRID_INDEX(x, y - 1)];

				edge->gradHeight = vec3f(0.0f, 0.0f, (ngb->height - cell->height) * -1.0f);
				cell->neighbors[cell->numNeighbors++] = GRID_INDEX(x, y - 1);

				mMinTerrainSlope = std::min(mMinTerrainSlope, std::fabs(edge->gradHeight.z));
				mMaxTerrainSlope = std::max(mMaxTerrainSlope, std::fabs(edge->gradHeight.z));

				mHeightDeltaVisData[idx * NUM_DIRS + dir] = edge->gradHeight * ((mSquareSize / mDownScale) >> 1);
			}

			if (y < mHeight - 1) {
				dir = DIR_S;

				edge = &mInitEdges[cell->edges[dir]];
				ngb = &mInitCells[GRID_INDEX(x, y + 1)];

				edge->gradHeight = vec3f(0.0f, 0.0f, (ngb->height - cell->height));
				cell->neighbors[cell->numNeighbors++] = GRID_INDEX(x, y + 1);

				mMinTerrainSlope = std::min(mMinTerrainSlope, std::fabs(edge->gradHeight.z));
				mMaxTerrainSlope = std::max(mMaxTerrainSlope, std::fabs(edge->gradHeight.z));

				mHeightDeltaVisData[idx * NUM_DIRS + dir] = edge->gradHeight * ((mSquareSize / mDownScale) >> 1);
			}

			if (x > 0) {
				dir = DIR_W;

				edge = &mInitEdges[cell->edges[dir]];
				ngb = &mInitCells[GRID_INDEX(x - 1, y)];

				edge->gradHeight = vec3f((ngb->height - cell->height) * -1.0f, 0.0f, 0.0f);
				cell->neighbors[cell->numNeighbors++] = GRID_INDEX(x - 1, y);

				mMinTerrainSlope = std::min(mMinTerrainSlope, std::fabs(edge->gradHeight.x));
				mMaxTerrainSlope = std::max(mMaxTerrainSlope, std::fabs(edge->gradHeight.x));

				mHeightDeltaVisData[idx * NUM_DIRS + dir] = edge->gradHeight * ((mSquareSize / mDownScale) >> 1);
			}

			if (x < mWidth - 1) {
				dir = DIR_E;

				edge = &mInitEdges[cell->edges[dir]];
				ngb = &mInitCells[GRID_INDEX(x + 1, y)];

				edge->gradHeight = vec3f((ngb->height - cell->height), 0.0f, 0.0f);
				cell->neighbors[cell->numNeighbors++] = GRID_INDEX(x + 1, y);

				mMinTerrainSlope = std::min(mMinTerrainSlope, std::fabs(edge->gradHeight.x));
				mMaxTerrainSlope = std::max(mMaxTerrainSlope, std::fabs(edge->gradHeight.x));

				mHeightDeltaVisData[idx * NUM_DIRS + dir] = edge->gradHeight * ((mSquareSize / mDownScale) >> 1);
			}
		}
	}

	// initialize both buffers with a copy of the static-global grid state
	mBuffers[mFrontBufferIdx].cells.assign(mInitCells.begin(), mInitCells.end());
	mBuffers[mFrontBufferIdx].edges.assign(mInitEdges.begin(), mInitEdges.end());
	mBuffers[mBackBufferIdx].cells.assign(mInitCells.begin(), mInitCells.end());
	mBuffers[mBackBufferIdx].edges.assign(mInitEdges.begin(), mInitEdges.end());
}

void Grid::Reset() {
	// at the start of every frame, restore both grid buffers (cells
	// and edges) to the blank initial-state again from the backups
	// made in Init to undo the dynamic global (and per-group) data
	// write-operations
	// NOTE: can we avoid this with triple-buffering?
	numResets += 1;
	mBuffers[mFrontBufferIdx].cells.assign(mInitCells.begin(), mInitCells.end());
	mBuffers[mFrontBufferIdx].edges.assign(mInitEdges.begin(), mInitEdges.end());
	mBuffers[mBackBufferIdx].cells.assign(mInitCells.begin(), mInitCells.end());
	mBuffers[mBackBufferIdx].edges.assign(mInitEdges.begin(), mInitEdges.end());

	for (std::set<unsigned int>::const_iterator it = mTouchedCells.begin(); it != mTouchedCells.end(); ++it) {
		const unsigned int idx = *it;

		mDensityVisData[idx]     = 0.0f;
		mAvgVelocityVisData[idx] = NVECf;
	}

	mTouchedCells.clear();

	mMaxDensity = -std::numeric_limits<float>::max();
}



void Grid::AddDensityAndVelocity(const vec3f& pos, const vec3f& vel) {
	const vec3f posf = vec3f(pos.x / mSquareSize, 0.0f, pos.z / mSquareSize);
	const vec3i posi = WorldPosToGridIdx(pos);

	const unsigned int i = std::max(1, std::min(int(mWidth - 1), (posf.x > (posi.x + 0.5f))? posi.x + 1: posi.x));
	const unsigned int j = std::max(1, std::min(int(mHeight - 1), (posf.z > (posi.z + 0.5f))? posi.z + 1: posi.z));

	PFFG_ASSERT(i > 0 && j > 0 && i < mWidth && j < mHeight);
 
	std::vector<Cell>& frontCells = mBuffers[mFrontBufferIdx].cells;
	std::vector<Cell>& backCells  = mBuffers[mBackBufferIdx ].cells;

	const unsigned int
		idxA = GRID_INDEX(i - 1, j - 1),
		idxB = GRID_INDEX(i    , j - 1),
		idxC = GRID_INDEX(i    , j    ),
		idxD = GRID_INDEX(i - 1, j    );

	Cell *Af = &frontCells[idxA], *Ab = &backCells[idxA]; mTouchedCells.insert(idxA);
	Cell *Bf = &frontCells[idxB], *Bb = &backCells[idxB]; mTouchedCells.insert(idxB);
	Cell *Cf = &frontCells[idxC], *Cb = &backCells[idxC]; mTouchedCells.insert(idxC);
	Cell *Df = &frontCells[idxD], *Db = &backCells[idxD]; mTouchedCells.insert(idxD);

	// add velocity (NOTE: why only to C?)
	Cf->avgVelocity += vel;
	Cb->avgVelocity += vel;

	// compute delta-X and delta-Y
	const float dX = posf.x - Af->x + 0.5f;
	const float dY = posf.z - Af->y + 0.5f;

	// splat the density
	Af->density += powf(std::min<float>(1.0f - dX, 1.0f - dY), EXP_DENSITY); Ab->density = Af->density;
	Bf->density += powf(std::min<float>(       dX, 1.0f - dY), EXP_DENSITY); Bb->density = Bf->density;
	Cf->density += powf(std::min<float>(       dX,        dY), EXP_DENSITY); Cb->density = Cf->density;
	Df->density += powf(std::min<float>(1.0f - dX,        dY), EXP_DENSITY); Db->density = Df->density;

	mMaxDensity = std::max(mMaxDensity, Af->density);
	mMaxDensity = std::max(mMaxDensity, Bf->density);
	mMaxDensity = std::max(mMaxDensity, Cf->density);
	mMaxDensity = std::max(mMaxDensity, Df->density);
}

void Grid::ComputeAvgVelocity() {
	std::vector<Cell>& frontCells = mBuffers[mFrontBufferIdx].cells;
	std::vector<Cell>& backCells  = mBuffers[mBackBufferIdx ].cells;

	for (std::set<unsigned int>::const_iterator it = mTouchedCells.begin(); it != mTouchedCells.end(); ++it) {
		const unsigned int idx = *it;

		Cell* cf = &frontCells[idx];
		Cell* cb = &backCells[idx];

		// normalise the densities, so comparisons with
		// MIN_DENSITY and MAX_DENSITY are well-defined
		// when constructing the speed-field
		cf->density     /= mMaxDensity;
		cf->avgVelocity /= cf->density;
		cb->avgVelocity  = cf->avgVelocity;

		mDensityVisData[idx]     = cf->density;
		mAvgVelocityVisData[idx] = cf->avgVelocity;
	}
}



void Grid::ComputeSpeedAndUnitCost(unsigned int groupID, Cell* cell) {
	const static float speedWeight      = 1.0f;
	const static float discomfortWeight = 100.0f;

	const unsigned int cellGridIdx = GRID_INDEX(cell->x, cell->y);
	const vec3f& cellWorldPos = GridIdxToWorldPos(cell);

	const std::vector<Cell      >& frontCells = mBuffers[mFrontBufferIdx].cells;
	const std::vector<Cell::Edge>& frontEdges = mBuffers[mFrontBufferIdx].edges;

	for (unsigned int dir = 0; dir < NUM_DIRS; dir++) {
		const vec3i&       ngbCellIdx3D = WorldPosToGridIdx(cellWorldPos + mDirVectors[dir] * mMaxGroupRadius);
		const unsigned int ngbCellIdx1D = GRID_INDEX(ngbCellIdx3D.x, ngbCellIdx3D.z);

		PFFG_ASSERT(ngbCellIdx1D < frontCells.size());

		const Cell* ngbCell = &frontCells[ngbCellIdx1D];
		const Cell::Edge* edge = &frontEdges[ cell->edges[dir] ];

		// compute the speed- and unit-cost fields
		// TODO:
		//    evaluate speed and discomfort at cell into which
		//    an agent would move if it chose direction <dir>
		//
		//    properly set discomfort for <cell> for this group (maybe via UI?)
		//    cell->discomfort = 0.0f * ((cell->x * cell->x) + (cell->y * cell->y));
		// NOTE: engine slopes should be in the same format as CC slopes?
		// NOTE:
		//    if the flow-speed is zero in a region, then the cost
		//    for cells with normalised density >= MAX_DENSITY will
		//    be infinite everywhere and the potential-field update
		//    can trigger asserts; this will also happen for cells
		//    with density <= MIN_DENSITY whenever topologicalSpeed
		//    is less than or equal to zero
		//
		const float directionalSlope  = edge->gradHeight.dot2D(mDirVectors[dir]);
		const float densitySpeedScale = (ngbCell->density - MIN_DENSITY) / (MAX_DENSITY - MIN_DENSITY);
		const float slopeSpeedScale   = (directionalSlope - mMinTerrainSlope) / (mMaxTerrainSlope - mMinTerrainSlope);
		const float topologicalSpeed  = mMaxGroupSpeed + CLAMP(slopeSpeedScale, -1.0f, 1.0f) * (mMinGroupSpeed - mMaxGroupSpeed);
		const float flowSpeed         = ngbCell->avgVelocity.dot2D(mDirVectors[dir]);
		const float interpolatedSpeed = topologicalSpeed + densitySpeedScale * (topologicalSpeed - flowSpeed);

		float speed = interpolatedSpeed;
		float cost = 0.0f;

		if (ngbCell->density >= MAX_DENSITY) { speed = flowSpeed; }
		if (ngbCell->density <= MIN_DENSITY) { speed = topologicalSpeed; }

		if (std::fabs(speed) > 0.1f) {
			cost = (speedWeight * speed + discomfortWeight * cell->discomfort) / speed;
		} else {
			// should this case be allowed to happen?
			// (infinite costs very heavily influence
			// behavior of potential-field generation)
			//
			// cost = std::numeric_limits<float>::infinity();
			// cost = std::numeric_limits<float>::max();
			speed = 0.1f;
			cost  = (speedWeight * speed + discomfortWeight * cell->discomfort) / speed;
		}

		cell->speed[dir] = speed;
		cell->cost[dir] = cost;

		// NOTE: how best to visualize these textures? one channel per direction?
		mSpeedVisData[groupID][cellGridIdx * NUM_DIRS + dir] = speed;
		mCostVisData[groupID][cellGridIdx * NUM_DIRS + dir] = cost;
	}

	mDiscomfortVisData[groupID][cellGridIdx] = cell->discomfort;
}

void Grid::UpdateGroupPotentialField(unsigned int groupID, const std::set<unsigned int>& goalIDs, const std::set<unsigned int>& objectIDs) {
	PFFG_ASSERT(!goalIDs.empty());
	PFFG_ASSERT(mCandidates.empty());

	// cycle the buffers so the per-group variables of the
	// previously processed group do not influence this one
	//
	// NOTE: this must be done here rather than at the end
	// of the update so that UpdateSimObjectLocation reads
	// from the correct buffer
	mFrontBufferIdx = (mFrontBufferIdx + 1) & 1;
	mBackBufferIdx = (mBackBufferIdx + 1) & 1;

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

	std::vector<float>& potVisData      = mPotentialVisData[groupID];
	std::vector<vec3f>& velVisData      = mVelocityVisData[groupID];
	std::vector<vec3f>& potDeltaVisData = mPotentialDeltaVisData[groupID];

	std::vector<Cell      >& frontCells = mBuffers[mFrontBufferIdx].cells;
	std::vector<Cell      >& backCells  = mBuffers[mBackBufferIdx ].cells;
	std::vector<Cell::Edge>& frontEdges = mBuffers[mFrontBufferIdx].edges;
	std::vector<Cell::Edge>& backEdges  = mBuffers[mBackBufferIdx ].edges;

	Cell* frontCell = NULL;
	Cell* backCell = NULL;

	numInfinitePotentialCases = 0;
	numIllegalDirectionCases = 0;

	unsigned int cellIdx = 0;

	// add goal-cells to the known set and their neighbors to the candidate-set
	for (std::set<unsigned int>::const_iterator it = goalIDs.begin(); it != goalIDs.end(); ++it) {
		cellIdx = *it;

		frontCell = &frontCells[cellIdx];
		backCell = &backCells[cellIdx];

		frontCell->known = true;
		frontCell->candidate = true;
		frontCell->potential = 0.0f;
		backCell->ResetGroupVars();

		ComputeSpeedAndUnitCost(groupID, frontCell);
		UpdateCandidates(groupID, frontCell);

		potVisData[cellIdx] = (frontCell->potential == std::numeric_limits<float>::infinity())? -1.0f: frontCell->potential;
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
		frontCell = mCandidates.top();
		frontCell->known = true;

		cellIdx = GRID_INDEX(frontCell->x, frontCell->y);

		backCell = &backCells[cellIdx];
		backCell->ResetGroupVars();

		UpdateCandidates(groupID, frontCell);

		frontEdges[ frontCell->edges[DIR_N] ].velocity = (frontCell->GetNormalisedPotentialGradient(frontEdges, DIR_N) * -frontCell->speed[DIR_N]);
		frontEdges[ frontCell->edges[DIR_S] ].velocity = (frontCell->GetNormalisedPotentialGradient(frontEdges, DIR_S) * -frontCell->speed[DIR_S]);
		frontEdges[ frontCell->edges[DIR_E] ].velocity = (frontCell->GetNormalisedPotentialGradient(frontEdges, DIR_E) * -frontCell->speed[DIR_E]);
		frontEdges[ frontCell->edges[DIR_W] ].velocity = (frontCell->GetNormalisedPotentialGradient(frontEdges, DIR_W) * -frontCell->speed[DIR_W]);
		backEdges[ frontCell->edges[DIR_N] ].velocity = NVECf;
		backEdges[ frontCell->edges[DIR_S] ].velocity = NVECf;
		backEdges[ frontCell->edges[DIR_E] ].velocity = NVECf;
		backEdges[ frontCell->edges[DIR_W] ].velocity = NVECf;

		potVisData[cellIdx] = (frontCell->potential == std::numeric_limits<float>::infinity())? -1.0f: frontCell->potential;
		velVisData[cellIdx * NUM_DIRS + DIR_N] = frontEdges[ frontCell->edges[DIR_N] ].velocity * (mSquareSize >> 1);
		velVisData[cellIdx * NUM_DIRS + DIR_S] = frontEdges[ frontCell->edges[DIR_S] ].velocity * (mSquareSize >> 1);
		velVisData[cellIdx * NUM_DIRS + DIR_E] = frontEdges[ frontCell->edges[DIR_E] ].velocity * (mSquareSize >> 1);
		velVisData[cellIdx * NUM_DIRS + DIR_W] = frontEdges[ frontCell->edges[DIR_W] ].velocity * (mSquareSize >> 1);
		potDeltaVisData[cellIdx * NUM_DIRS + DIR_N] = frontEdges[ frontCell->edges[DIR_N] ].gradPotential * (mSquareSize >> 1);
		potDeltaVisData[cellIdx * NUM_DIRS + DIR_S] = frontEdges[ frontCell->edges[DIR_S] ].gradPotential * (mSquareSize >> 1);
		potDeltaVisData[cellIdx * NUM_DIRS + DIR_E] = frontEdges[ frontCell->edges[DIR_E] ].gradPotential * (mSquareSize >> 1);
		potDeltaVisData[cellIdx * NUM_DIRS + DIR_W] = frontEdges[ frontCell->edges[DIR_W] ].gradPotential * (mSquareSize >> 1);

		mCandidates.pop();
	}

	PFFG_ASSERT(numInfinitePotentialCases == 0 && numIllegalDirectionCases == 0);
}

void Grid::UpdateCandidates(unsigned int groupID, const Cell* parent) {
	std::vector<Cell      >& frontCells = mBuffers[mFrontBufferIdx].cells;
	std::vector<Cell      >& backCells  = mBuffers[mBackBufferIdx ].cells;
	std::vector<Cell::Edge>& frontEdges = mBuffers[mFrontBufferIdx].edges;
	std::vector<Cell::Edge>& backEdges  = mBuffers[mBackBufferIdx ].edges;

	static float       dirCosts[NUM_DIRS] = {0.0f};
	static bool        dirValid[NUM_DIRS] = {false};
	static const Cell* dirCells[NUM_DIRS] = {NULL};

	const Cell* minPotCellPtrX = NULL;
	const Cell* minPotCellPtrY = NULL;
	int         minPotCellDirX = -1;
	int         minPotCellDirY = -1;

	Cell::Edge* frontEdgeX = NULL;
	Cell::Edge* frontEdgeY = NULL;
	Cell::Edge* backEdgeX  = NULL;
	Cell::Edge* backEdgeY  = NULL;

	for (unsigned int i = 0; i < parent->numNeighbors; i++) {
		const unsigned int ngbIdx = parent->neighbors[i];

		Cell* frontNgb = &frontCells[ngbIdx];
		Cell* backNgb = &backCells[ngbIdx];

		backNgb->ResetGroupVars();

		if (frontNgb->known || frontNgb->candidate) {
			continue;
		}

		ComputeSpeedAndUnitCost(groupID, frontNgb);

		dirCells[DIR_N] = (frontNgb->y >           0) ? &frontCells[GRID_INDEX(frontNgb->x    , frontNgb->y - 1)] : NULL;
		dirCells[DIR_S] = (frontNgb->y < mHeight - 1) ? &frontCells[GRID_INDEX(frontNgb->x    , frontNgb->y + 1)] : NULL;
		dirCells[DIR_E] = (frontNgb->x < mWidth -  1) ? &frontCells[GRID_INDEX(frontNgb->x + 1, frontNgb->y    )] : NULL;
		dirCells[DIR_W] = (frontNgb->x >           0) ? &frontCells[GRID_INDEX(frontNgb->x - 1, frontNgb->y    )] : NULL;

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
				frontNgb->potential = Potential2D(
					minPotCellPtrX->potential, frontNgb->cost[minPotCellDirX], 
					minPotCellPtrY->potential, frontNgb->cost[minPotCellDirY]
				);

				// the world-space direction of the gradient vector must always
				// match the direction along which the potential increases, but
				// for DIR_N and DIR_W these are inverted
				const float gradientX = (minPotCellPtrX->potential - frontNgb->potential);
				const float gradientY = (minPotCellPtrY->potential - frontNgb->potential);
				const float scaleX = (minPotCellDirX == DIR_W)? -1.0f: 1.0f;
				const float scaleY = (minPotCellDirY == DIR_N)? -1.0f: 1.0f;
				const vec3f gradient = vec3f(gradientX * scaleX, 0.0f, gradientY * scaleY);

				frontEdgeX = &frontEdges[frontNgb->edges[minPotCellDirX]];
				frontEdgeY = &frontEdges[frontNgb->edges[minPotCellDirY]];
				backEdgeX  = &backEdges[frontNgb->edges[minPotCellDirX]];
				backEdgeY  = &backEdges[frontNgb->edges[minPotCellDirY]];

				frontEdgeX->gradPotential = gradient;
				frontEdgeY->gradPotential = gradient;
				backEdgeX->gradPotential = NVECf;
				backEdgeY->gradPotential = NVECf;
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
					frontNgb->potential = Potential1D(minPotCellPtrY->potential, frontNgb->cost[minPotCellDirY]);

					const float gradientY = (minPotCellPtrY->potential - frontNgb->potential);
					const float scaleY = (minPotCellDirY == DIR_N)? -1.0f: 1.0f;
					const vec3f gradient = vec3f(0.0f, 0.0f, gradientY * scaleY);

					frontEdgeY = &frontEdges[frontNgb->edges[minPotCellDirY]];
					backEdgeY  = &backEdges[frontNgb->edges[minPotCellDirY]];

					frontEdgeY->gradPotential = gradient;
					backEdgeY->gradPotential = NVECf;
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
					frontNgb->potential = Potential1D(minPotCellPtrX->potential, frontNgb->cost[minPotCellDirX]);

					const float gradientX = (minPotCellPtrX->potential - frontNgb->potential);
					const float scaleX = (minPotCellDirX == DIR_W)? -1.0f: 1.0f;
					const vec3f gradient = vec3f(gradientX * scaleX, 0.0f, 0.0f);

					frontEdgeX = &frontEdges[frontNgb->edges[minPotCellDirX]];
					backEdgeX  = &backEdges[frontNgb->edges[minPotCellDirX]];

					frontEdgeX->gradPotential = gradient;
					backEdgeX->gradPotential = NVECf;
				} else {
					numIllegalDirectionCases += 1;
				}
			}
		}

		frontNgb->candidate = true;
		mCandidates.push(frontNgb);

		numInfinitePotentialCases += int(frontNgb->potential == std::numeric_limits<float>::infinity());
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

	const std::vector<Cell      >& frontCells = mBuffers[mFrontBufferIdx].cells;
	const std::vector<Cell::Edge>& frontEdges = mBuffers[mFrontBufferIdx].edges;

	const Cell* objectCell = &frontCells[objectCellID];
	const vec3f& objectCellVel = objectCell->GetInterpolatedVelocity(frontEdges, objectDir);

	PFFG_ASSERT_MSG(!(std::isnan(objectCellVel.x) || std::isnan(objectCellVel.y) || std::isnan(objectCellVel.z)), "Inf velocity-field for cell %u", cellIdx);
	PFFG_ASSERT_MSG(!(std::isinf(objectCellVel.x) || std::isinf(objectCellVel.y) || std::isinf(objectCellVel.z)), "NaN velocity-field for cell %u", cellIdx);

	if (objectCellVel.sqLen3D() > 0.01f) {
		#ifdef DIRECT_VELOCITY_FIELD_INTERPOLATION
		// TODO: smoother interpolation
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



vec3f Grid::Cell::GetNormalisedPotentialGradient(const std::vector<Cell::Edge>& gridEdges, unsigned int dir) const {
	const Edge* edge = &gridEdges[edges[dir]];
	const vec3f& edgeGradPot = edge->gradPotential;
	const float edgeGradPotLen = edgeGradPot.len2D();

	if (edgeGradPotLen > 0.0f) {
		return (edgeGradPot / edgeGradPotLen);
	}

	return NVECf;
}

vec3f Grid::Cell::GetInterpolatedVelocity(const std::vector<Cell::Edge>& gridEdges, const vec3f& dir) const {
	// <dir> always falls into one of four quadrants
	// therefore, we need to sample the speed-field
	// and potential-gradient along two of the four
	// cardinal (NSEW) directions and interpolate
	vec3f vel;

	float a = 0.0f;
	float b = 0.0f;

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

	vel += (gridEdges[ edges[i] ].velocity * a);
	vel += (gridEdges[ edges[j] ].velocity * b);

	return vel;
}



// convert a world-space position to <x, y> grid-indices
vec3i Grid::WorldPosToGridIdx(const vec3f& worldPos) const {
	const int gx = (worldPos.x / mSquareSize);
	const int gz = (worldPos.z / mSquareSize);
	const int cx = std::max(0, std::min(int(mWidth - 1), gx));
	const int cz = std::max(0, std::min(int(mHeight - 1), gz));
	return vec3i(cx, 0, cz);
}

// get the 1D grid-cell index corresponding to a world-space position
unsigned int Grid::WorldPosToCellID(const vec3f& worldPos) const {
	const vec3i& gridPos = WorldPosToGridIdx(worldPos);
	const unsigned int gridIdx = GRID_INDEX(gridPos.x, gridPos.z);

	PFFG_ASSERT_MSG(gridIdx < mInitCells.size(), "world(%2.2f, %2.2f) grid(%d, %d)", worldPos.x, worldPos.z, gridPos.x, gridPos.z);
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
}

void Grid::Cell::ResetGroupVars() {
	potential  = std::numeric_limits<float>::infinity();
	discomfort = 0.0f;

	known     = false;
	candidate = false;

	for (unsigned int dir = 0; dir < NUM_DIRS; dir++) {
		speed[dir] = 0.0f;
		cost[dir]  = 0.0f;
	}
}
