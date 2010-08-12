#include <cmath>
#include <algorithm>
#include <limits>

#include "./Grid.hpp"
#include "../../Ext/ICallOutHandler.hpp"
#include "../../Sim/SimObjectDef.hpp"
#include "../../System/Debugger.hpp"

#define GRID_INDEX(x, y) (((y) * (mWidth)) + (x))
#define ELEVATION(x, y) (mCOH->GetCenterHeightMap()[(mDownScale * (y)) * (mDownScale * mWidth) + (mDownScale * (x))])
#define CLAMP(f, fmin, fmax) std::max(fmin, std::min(fmax, (f)))

void Grid::AddGroup(unsigned int groupID) {
	mDiscomfortVisData[groupID] = std::vector<float>();
	mDiscomfortVisData[groupID].resize(mWidth * mHeight);
	mSpeedVisData[groupID] = std::vector<float>();
	mSpeedVisData[groupID].resize(mWidth * mHeight * NUM_DIRECTIONS);
	mCostVisData[groupID] = std::vector<float>();
	mCostVisData[groupID].resize(mWidth * mHeight * NUM_DIRECTIONS);

	mPotentialVisData[groupID] = std::vector<float>();
	mPotentialVisData[groupID].resize(mWidth * mHeight);

	mVelocityVisData[groupID] = std::vector<vec3f>();
	mVelocityVisData[groupID].resize(mWidth * mHeight * NUM_DIRECTIONS);

	mPotentialDeltaVisData[groupID] = std::vector<vec3f>();
	mPotentialDeltaVisData[groupID].resize(mWidth * mHeight * NUM_DIRECTIONS);
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



void Grid::Kill(const std::map<unsigned int, std::set<unsigned int> >& groupIDs) {
	// clear the global scalar field visualisation data
	mDensityVisData.clear();
	mHeightVisData.clear();

	// clear the global vector field visualisation data
	mAvgVelocityVisData.clear();
	mHeightDeltaVisData.clear();

	// clear the per-group visualisation data
	for (std::map<unsigned int, std::set<unsigned int> >::const_iterator it = groupIDs.begin(); it != groupIDs.end(); ++it) {
		DelGroup(it->first);
	}

	mTouchedCells.clear();
	mInitCells.clear();
	mInitEdges.clear();
	mBuffers[mFrontBufferIdx].cells.clear();
	mBuffers[mFrontBufferIdx].edges.clear();
	mBuffers[mBackBufferIdx].cells.clear();
	mBuffers[mBackBufferIdx].edges.clear();
}

void Grid::Init(const int inDownScale, ICallOutHandler* inCOH) {
	PFFG_ASSERT(inDownScale >= 1);

	// NOTE: if mDownScale > 1, the engine's height-map must be downsampled
	mDownScale  = inDownScale;
	mCOH        = inCOH;
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
	const float squareSizeFlt = mSquareSize;

	// visualisation data for global scalar fields
	mDensityVisData.resize(numCells);
	mHeightVisData.resize(numCells);

	// visualisation data for global vector fields
	mAvgVelocityVisData.resize(numCells);
	mHeightDeltaVisData.resize(numCells * NUM_DIRECTIONS);


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

			curCell->edges[DIRECTION_WEST]  = edgeIdxW;
			curCell->edges[DIRECTION_NORTH] = edgeIdxN;

			// bind the east face of the cell west of the current cell
			if (x > 0) {
				cellIdx = GRID_INDEX(x - 1, y);
				PFFG_ASSERT(cellIdx < mInitCells.size());

				Cell* westCell = &mInitCells[cellIdx];
				westCell->edges[DIRECTION_EAST] = edgeIdxW;
			}

			// bind the south face of the cell north of the current cell
			if (y > 0) {
				cellIdx = GRID_INDEX(x, y - 1);
				PFFG_ASSERT(cellIdx < mInitCells.size());

				Cell* northCell = &mInitCells[cellIdx];
				northCell->edges[DIRECTION_SOUTH] = edgeIdxN;
			}

			// bind a new face to the southern face of the border cell
			if (y == mHeight - 1) {
				mInitEdges.push_back(Grid::Cell::Edge());
				curCell->edges[DIRECTION_SOUTH] = mInitEdges.size() - 1;
			}

			// bind a new face to the eastern face of the border cell
			if (x == mWidth - 1) {
				mInitEdges.push_back(Grid::Cell::Edge());
				curCell->edges[DIRECTION_EAST] = mInitEdges.size() - 1;
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
			Cell::Edge* edge = NULL;

			if (y > 0) {
				dir = DIRECTION_NORTH;

				edge = &mInitEdges[cell->edges[dir]];
				edge->gradHeight = vec3f(-squareSizeFlt, 0.0f, mInitCells[GRID_INDEX(x, y - 1)].height - cell->height);
				cell->neighbors[cell->numNeighbors++] = GRID_INDEX(x, y - 1);

				mMinTerrainSlope = std::min(mMinTerrainSlope, std::fabs(edge->gradHeight.z));
				mMaxTerrainSlope = std::max(mMaxTerrainSlope, std::fabs(edge->gradHeight.z));
				mHeightDeltaVisData[idx * NUM_DIRECTIONS + dir] = edge->gradHeight;
			}

			if (y < mHeight - 1) {
				dir = DIRECTION_SOUTH;

				edge = &mInitEdges[cell->edges[dir]];
				edge->gradHeight = vec3f( squareSizeFlt, 0.0f, mInitCells[GRID_INDEX(x, y + 1)].height - cell->height);
				cell->neighbors[cell->numNeighbors++] = GRID_INDEX(x, y + 1);

				mMinTerrainSlope = std::min(mMinTerrainSlope, std::fabs(edge->gradHeight.z));
				mMaxTerrainSlope = std::max(mMaxTerrainSlope, std::fabs(edge->gradHeight.z));
				mHeightDeltaVisData[idx * NUM_DIRECTIONS + dir] = edge->gradHeight;
			}

			if (x > 0) {
				dir = DIRECTION_WEST;

				edge = &mInitEdges[cell->edges[dir]];
				edge->gradHeight = vec3f(mInitCells[GRID_INDEX(x - 1, y)].height - cell->height, 0.0f, -squareSizeFlt);
				cell->neighbors[cell->numNeighbors++] = GRID_INDEX(x - 1, y);

				mMinTerrainSlope = std::min(mMinTerrainSlope, std::fabs(edge->gradHeight.x));
				mMaxTerrainSlope = std::max(mMaxTerrainSlope, std::fabs(edge->gradHeight.x));
				mHeightDeltaVisData[idx * NUM_DIRECTIONS + dir] = edge->gradHeight;
			}

			if (x < mWidth - 1) {
				dir = DIRECTION_EAST;

				edge = &mInitEdges[cell->edges[dir]];
				edge->gradHeight = vec3f(mInitCells[GRID_INDEX(x + 1, y)].height - cell->height, 0.0f, squareSizeFlt);
				cell->neighbors[cell->numNeighbors++] = GRID_INDEX(x + 1, y);

				mMinTerrainSlope = std::min(mMinTerrainSlope, std::fabs(edge->gradHeight.x));
				mMaxTerrainSlope = std::max(mMaxTerrainSlope, std::fabs(edge->gradHeight.x));
				mHeightDeltaVisData[idx * NUM_DIRECTIONS + dir] = edge->gradHeight;
			}
		}
	}

	// initialize both buffers with a copy of the static-global grid state
	mBuffers[mFrontBufferIdx].cells.assign(mInitCells.begin(), mInitCells.end());
	mBuffers[mFrontBufferIdx].edges.assign(mInitEdges.begin(), mInitEdges.end());
	mBuffers[mBackBufferIdx].cells.assign(mInitCells.begin(), mInitCells.end());
	mBuffers[mBackBufferIdx].edges.assign(mInitEdges.begin(), mInitEdges.end());
}



void Grid::AddDensityAndVelocity(const vec3f& inPos, const vec3f& inVel) {
	const vec3f posf = vec3f(inPos.x / mSquareSize, 0.0f, inPos.z / mSquareSize);
	const vec3i posi = World2Grid(inPos);

	const int i = (posf.x > posi.x + 0.5f) ? posi.x + 1 : posi.x;
	const int j = (posf.z > posi.z + 0.5f) ? posi.z + 1 : posi.z;

	PFFG_ASSERT(i > 0 && j > 0 && i < mWidth && j < mHeight);
 
	std::vector<Cell>& frontCells = mBuffers[mFrontBufferIdx].cells;
	std::vector<Cell>& backCells = mBuffers[mBackBufferIdx].cells;

	const unsigned int
		idxA = GRID_INDEX(i - 1, j - 1),
		idxB = GRID_INDEX(i    , j - 1),
		idxC = GRID_INDEX(i    , j    ),
		idxD = GRID_INDEX(i - 1, j    );

	Cell *Af = &frontCells[idxA], *Ab = &backCells[idxA]; mTouchedCells.insert(idxA);
	Cell *Bf = &frontCells[idxB], *Bb = &backCells[idxB]; mTouchedCells.insert(idxB);
	Cell *Cf = &frontCells[idxC], *Cb = &backCells[idxC]; mTouchedCells.insert(idxC);
	Cell *Df = &frontCells[idxD], *Db = &backCells[idxD]; mTouchedCells.insert(idxD);

	// add velocity (why only to C?)
	Cf->avgVelocity += inVel;
	Cb->avgVelocity += inVel;

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
	std::vector<Cell>& backCells = mBuffers[mBackBufferIdx].cells;

	for (std::set<unsigned int>::iterator it = mTouchedCells.begin(); it != mTouchedCells.end(); ++it) {
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
	const static vec3f dirVectors[NUM_DIRECTIONS] = {
		-ZVECf, // N (world-space)
		 XVECf, // E (world-space)
		 ZVECf, // S (world-space)
		-XVECf  // W (world-space)
	};

	const static float speedWeight      = 1.0f;
	const static float discomfortWeight = 100.0f;

	const unsigned int cellGridIdx = GRID_INDEX(cell->x, cell->y);
	const vec3f& cellWorldPos = Grid2World(cell);

	const std::vector<Cell      >& frontCells = mBuffers[mFrontBufferIdx].cells;
	const std::vector<Cell::Edge>& frontEdges = mBuffers[mFrontBufferIdx].edges;

	for (unsigned int dir = 0; dir < NUM_DIRECTIONS; dir++) {
		const vec3i& ngbGridPos = World2Grid(cellWorldPos + dirVectors[dir] * mMaxGroupRadius);
		const unsigned int ngbGridIdx = GRID_INDEX(ngbGridPos.x, ngbGridPos.z);

		PFFG_ASSERT(ngbGridIdx < frontCells.size());

		const Cell* ngbCell = &frontCells[ngbGridIdx];
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
		//    be FLOAT_MAX everywhere and the potential-field update
		//    can trigger asserts; this will also happen for cells
		//    with density <= MIN_DENSITY whenever topologicalSpeed
		//    is less than or equal to zero
		//
		//    (the potential-field will also become invisible due
		//    to the normalisation by FLOAT_MAX)
		//
		const float directionalSlope  = edge->gradHeight.dot2D(dirVectors[dir]);
		const float densitySpeedScale = (ngbCell->density - MIN_DENSITY) / (MAX_DENSITY - MIN_DENSITY);
		const float slopeSpeedScale   = (directionalSlope - mMinTerrainSlope) / (mMaxTerrainSlope - mMinTerrainSlope);
		const float topologicalSpeed  = mMaxGroupSpeed + CLAMP(slopeSpeedScale, -1.0f, 1.0f) * (mMinGroupSpeed - mMaxGroupSpeed);
		const float flowSpeed         = ngbCell->avgVelocity.dot2D(dirVectors[dir]);
		const float interpolatedSpeed = topologicalSpeed + densitySpeedScale * (topologicalSpeed - flowSpeed);

		float speed = interpolatedSpeed;
		float cost = 0.0f;

	//	if (ngbCell->density >= MAX_DENSITY) { speed = flowSpeed; }
		if (ngbCell->density <= MIN_DENSITY) { speed = topologicalSpeed; }

		if (speed > 0.01f) {
			cost = (speedWeight * speed + discomfortWeight * cell->discomfort) / speed;
		} else {
			cost = std::numeric_limits<float>::max();
		}

		cell->speed[dir] = speed;
		cell->cost[dir] = cost;

		// NOTE: do we even want to visualize these as textures?
		mSpeedVisData[groupID][cellGridIdx * NUM_DIRECTIONS + dir] = speed;
		mCostVisData[groupID][cellGridIdx * NUM_DIRECTIONS + dir] = cost;
	}

	mDiscomfortVisData[groupID][cellGridIdx] = cell->discomfort;
}

void Grid::UpdateGroupPotentialField(unsigned int groupID, const std::vector<unsigned int>& inGoalCells, const std::set<unsigned int>& inSimObjectIds) {
	PFFG_ASSERT(!inGoalCells.empty());
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

	for (std::set<unsigned int>::iterator i = inSimObjectIds.begin(); i != inSimObjectIds.end(); i++) {
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
	std::vector<Cell      >& backCells  = mBuffers[mBackBufferIdx].cells;
	std::vector<Cell::Edge>& frontEdges = mBuffers[mFrontBufferIdx].edges;
	std::vector<Cell::Edge>& backEdges  = mBuffers[mBackBufferIdx].edges;

	Cell* frontCell = NULL;
	Cell* backCell = NULL;

	unsigned int cellIdx = 0;

	// add goal-cells to the known set and their neighbors to the candidate-set
	for (size_t i = 0; i < inGoalCells.size(); i++) {
		cellIdx = inGoalCells[i];

		frontCell = &frontCells[cellIdx];
		backCell = &backCells[cellIdx];

		frontCell->known = true;
		frontCell->candidate = true;
		frontCell->potential = 0.0f;
		backCell->ResetGroupVars();

		ComputeSpeedAndUnitCost(groupID, frontCell);
		UpdateCandidates(groupID, frontCell);

		potVisData[cellIdx] = frontCell->potential;
		velVisData[cellIdx * NUM_DIRECTIONS + DIRECTION_NORTH] = NVECf;
		velVisData[cellIdx * NUM_DIRECTIONS + DIRECTION_SOUTH] = NVECf;
		velVisData[cellIdx * NUM_DIRECTIONS + DIRECTION_EAST ] = NVECf;
		velVisData[cellIdx * NUM_DIRECTIONS + DIRECTION_WEST ] = NVECf;
		potDeltaVisData[cellIdx * NUM_DIRECTIONS + DIRECTION_NORTH] = NVECf;
		potDeltaVisData[cellIdx * NUM_DIRECTIONS + DIRECTION_SOUTH] = NVECf;
		potDeltaVisData[cellIdx * NUM_DIRECTIONS + DIRECTION_EAST ] = NVECf;
		potDeltaVisData[cellIdx * NUM_DIRECTIONS + DIRECTION_WEST ] = NVECf;
	}

	while (!mCandidates.empty()) {
		frontCell = mCandidates.top();
		frontCell->known = true;

		cellIdx = GRID_INDEX(frontCell->x, frontCell->y);

		backCell = &backCells[cellIdx];
		backCell->ResetGroupVars();

		UpdateCandidates(groupID, frontCell);

		frontEdges[ frontCell->edges[DIRECTION_NORTH] ].velocity = (frontCell->GetNormalisedPotentialGradient(frontEdges, DIRECTION_NORTH) * -frontCell->speed[DIRECTION_NORTH]);
		frontEdges[ frontCell->edges[DIRECTION_SOUTH] ].velocity = (frontCell->GetNormalisedPotentialGradient(frontEdges, DIRECTION_SOUTH) * -frontCell->speed[DIRECTION_SOUTH]);
		frontEdges[ frontCell->edges[DIRECTION_EAST ] ].velocity = (frontCell->GetNormalisedPotentialGradient(frontEdges, DIRECTION_EAST ) * -frontCell->speed[DIRECTION_EAST ]);
		frontEdges[ frontCell->edges[DIRECTION_WEST ] ].velocity = (frontCell->GetNormalisedPotentialGradient(frontEdges, DIRECTION_WEST ) * -frontCell->speed[DIRECTION_WEST ]);
		backEdges[ frontCell->edges[DIRECTION_NORTH] ].velocity = NVECf;
		backEdges[ frontCell->edges[DIRECTION_SOUTH] ].velocity = NVECf;
		backEdges[ frontCell->edges[DIRECTION_EAST ] ].velocity = NVECf;
		backEdges[ frontCell->edges[DIRECTION_WEST ] ].velocity = NVECf;

		potVisData[cellIdx] = frontCell->potential;
		velVisData[cellIdx * NUM_DIRECTIONS + DIRECTION_NORTH] = frontEdges[ frontCell->edges[DIRECTION_NORTH] ].velocity;
		velVisData[cellIdx * NUM_DIRECTIONS + DIRECTION_SOUTH] = frontEdges[ frontCell->edges[DIRECTION_SOUTH] ].velocity;
		velVisData[cellIdx * NUM_DIRECTIONS + DIRECTION_EAST ] = frontEdges[ frontCell->edges[DIRECTION_EAST ] ].velocity;
		velVisData[cellIdx * NUM_DIRECTIONS + DIRECTION_WEST ] = frontEdges[ frontCell->edges[DIRECTION_WEST ] ].velocity;
		potDeltaVisData[cellIdx * NUM_DIRECTIONS + DIRECTION_NORTH] = frontEdges[ frontCell->edges[DIRECTION_NORTH] ].gradPotential;
		potDeltaVisData[cellIdx * NUM_DIRECTIONS + DIRECTION_SOUTH] = frontEdges[ frontCell->edges[DIRECTION_SOUTH] ].gradPotential;
		potDeltaVisData[cellIdx * NUM_DIRECTIONS + DIRECTION_EAST ] = frontEdges[ frontCell->edges[DIRECTION_EAST ] ].gradPotential;
		potDeltaVisData[cellIdx * NUM_DIRECTIONS + DIRECTION_WEST ] = frontEdges[ frontCell->edges[DIRECTION_WEST ] ].gradPotential;

		mCandidates.pop();
	}
}

void Grid::UpdateCandidates(unsigned int groupID, const Cell* inParent) {
	std::vector<Cell      >& frontCells = mBuffers[mFrontBufferIdx].cells;
	std::vector<Cell      >& backCells  = mBuffers[mBackBufferIdx].cells;
	std::vector<Cell::Edge>& frontEdges = mBuffers[mFrontBufferIdx].edges;
	std::vector<Cell::Edge>& backEdges  = mBuffers[mBackBufferIdx].edges;

	static float       dirCosts[NUM_DIRECTIONS] = {0.0f};
	static bool        dirValid[NUM_DIRECTIONS] = {false};
	static const Cell* dirCells[NUM_DIRECTIONS] = {NULL};

	const Cell* minPotCellPtrX = NULL;
	const Cell* minPotCellPtrY = NULL;
	int         minPotCellDirX = -1;
	int         minPotCellDirY = -1;

	for (unsigned int i = 0; i < inParent->numNeighbors; i++) {
		const unsigned int ngbIdx = inParent->neighbors[i];

		Cell* frontNgb = &frontCells[ngbIdx];
		Cell* backNgb = &backCells[ngbIdx];

		backNgb->ResetGroupVars();

		if (frontNgb->known || frontNgb->candidate) {
			continue;
		}

		ComputeSpeedAndUnitCost(groupID, frontNgb);

		dirCells[DIRECTION_NORTH] = (frontNgb->y >           0) ? &frontCells[GRID_INDEX(frontNgb->x    , frontNgb->y - 1)] : NULL;
		dirCells[DIRECTION_SOUTH] = (frontNgb->y < mHeight - 1) ? &frontCells[GRID_INDEX(frontNgb->x    , frontNgb->y + 1)] : NULL;
		dirCells[DIRECTION_WEST]  = (frontNgb->x >           0) ? &frontCells[GRID_INDEX(frontNgb->x - 1, frontNgb->y    )] : NULL;
		dirCells[DIRECTION_EAST]  = (frontNgb->x < mWidth -  1) ? &frontCells[GRID_INDEX(frontNgb->x + 1, frontNgb->y    )] : NULL;

		for (unsigned int dir = 0; dir < NUM_DIRECTIONS; dir++) {
			if (dirCells[dir] == NULL) {
				dirValid[dir] = false;
				dirCosts[dir] = std::numeric_limits<float>::infinity();
			} else {
				dirCosts[dir] = (dirCells[dir]->potential + dirCells[dir]->cost[dir]);
				dirValid[dir] = (dirCosts[dir] != std::numeric_limits<float>::infinity());
			}
		}

		const bool undefinedX = (!dirValid[DIRECTION_WEST] && !dirValid[DIRECTION_EAST]);
		const bool undefinedY = (!dirValid[DIRECTION_NORTH] && !dirValid[DIRECTION_SOUTH]);

		// at least one dimension must ALWAYS be defined
		PFFG_ASSERT((int(undefinedX) + int(undefinedY)) < 2);

		if (!undefinedX && !undefinedY) {
			// both dimensions are defined
			PFFG_ASSERT(dirValid[DIRECTION_NORTH] || dirValid[DIRECTION_SOUTH]);
			PFFG_ASSERT(dirValid[DIRECTION_EAST ] || dirValid[DIRECTION_WEST ]);

			if (dirCosts[DIRECTION_EAST] < dirCosts[DIRECTION_WEST]) {
				minPotCellDirX = DIRECTION_EAST;
				minPotCellPtrX = dirCells[DIRECTION_EAST];
			} else {
				minPotCellDirX = DIRECTION_WEST;
				minPotCellPtrX = dirCells[DIRECTION_WEST];
			}

			if (dirCosts[DIRECTION_NORTH] < dirCosts[DIRECTION_SOUTH]) {
				minPotCellDirY = DIRECTION_NORTH;
				minPotCellPtrY = dirCells[DIRECTION_NORTH];
			} else {
				minPotCellDirY = DIRECTION_SOUTH;
				minPotCellPtrY = dirCells[DIRECTION_SOUTH];
			}

			PFFG_ASSERT(minPotCellPtrX != NULL && minPotCellPtrY != NULL);

			frontNgb->potential = Potential2D(
				minPotCellPtrX->potential, frontNgb->cost[minPotCellDirX], 
				minPotCellPtrY->potential, frontNgb->cost[minPotCellDirY]
			);

			frontEdges[ frontNgb->edges[minPotCellDirX] ].gradPotential = vec3f(mSquareSize, 0.0f, (frontNgb->potential - minPotCellPtrX->potential));
			frontEdges[ frontNgb->edges[minPotCellDirY] ].gradPotential = vec3f((frontNgb->potential - minPotCellPtrY->potential), 0.0f, mSquareSize);
			frontEdges[ frontNgb->edges[minPotCellDirX] ].gradPotential.x *= ((minPotCellDirX == DIRECTION_EAST) ? -1.0f : 1.0f);
			frontEdges[ frontNgb->edges[minPotCellDirY] ].gradPotential.z *= ((minPotCellDirY == DIRECTION_NORTH)? -1.0f : 1.0f);
			backEdges[ frontNgb->edges[minPotCellDirX] ].gradPotential = NVECf;
			backEdges[ frontNgb->edges[minPotCellDirY] ].gradPotential = NVECf;

			// NOTE:
			//    gradHeight is stored as vec3f(squareSize, 0.0f, deltaHeight) for DIR_N and
			//    DIR_S, and as vec3f(deltaHeight, 0.0f, squareSize) for DIR_E and DIR_W
			//    this field is used to calculate the topological speed via dot2D operations
			//    therefore gradPotential "should" be stored the same way, but this makes no
			//    sense for rendering purposes (since all vectors end up in the xz-plane)
			//
			// frontNgb->edges[minPotCellDirX]->gradPotential = vec3f(mSquareSize, (frontNgb->potential - minPotCellPtrX->potential),        0.0f);
			// frontNgb->edges[minPotCellDirY]->gradPotential = vec3f(       0.0f, (frontNgb->potential - minPotCellPtrY->potential), mSquareSize);
			// frontNgb->edges[minPotCellDirX]->gradPotential.y *= ((minPotCellDirX == DIRECTION_EAST) ? -1.0f : 1.0f);
			// frontNgb->edges[minPotCellDirY]->gradPotential.y *= ((minPotCellDirY == DIRECTION_NORTH)? -1.0f : 1.0f);
		} else {
			if (undefinedX) {
				PFFG_ASSERT(dirValid[DIRECTION_NORTH] || dirValid[DIRECTION_SOUTH]);
				PFFG_ASSERT(!undefinedY);

				if (dirCosts[DIRECTION_NORTH] < dirCosts[DIRECTION_SOUTH]) {
					minPotCellDirY = DIRECTION_NORTH;
					minPotCellPtrY = dirCells[DIRECTION_NORTH];
				} else {
					minPotCellDirY = DIRECTION_SOUTH;
					minPotCellPtrY = dirCells[DIRECTION_SOUTH];
				}

				PFFG_ASSERT(minPotCellPtrY != NULL);

				frontNgb->potential = Potential1D(minPotCellPtrY->potential, frontNgb->cost[minPotCellDirY]);
				frontEdges[ frontNgb->edges[minPotCellDirY] ].gradPotential = vec3f((frontNgb->potential - minPotCellPtrY->potential), 0.0f, mSquareSize);
				frontEdges[ frontNgb->edges[minPotCellDirY] ].gradPotential.z *= ((minPotCellDirY == DIRECTION_NORTH)? -1.0f : 1.0f);
				backEdges[ frontNgb->edges[minPotCellDirY] ].gradPotential = NVECf;
			}

			if (undefinedY) {
				PFFG_ASSERT(dirValid[DIRECTION_EAST] || dirValid[DIRECTION_WEST]);
				PFFG_ASSERT(!undefinedX);

				if (dirCosts[DIRECTION_EAST] < dirCosts[DIRECTION_WEST]) {
					minPotCellDirX = DIRECTION_EAST;
					minPotCellPtrX = dirCells[DIRECTION_EAST];
				} else {
					minPotCellDirX = DIRECTION_WEST;
					minPotCellPtrX = dirCells[DIRECTION_WEST];
				}

				PFFG_ASSERT(minPotCellPtrX != NULL);

				frontNgb->potential = Potential1D(minPotCellPtrX->potential, frontNgb->cost[minPotCellDirX]);
				frontEdges[ frontNgb->edges[minPotCellDirX] ].gradPotential = vec3f(mSquareSize, 0.0f, (frontNgb->potential - minPotCellPtrX->potential));
				frontEdges[ frontNgb->edges[minPotCellDirX] ].gradPotential.x *= ((minPotCellDirX == DIRECTION_EAST)? -1.0f : 1.0f);
				backEdges[ frontNgb->edges[minPotCellDirX] ].gradPotential = NVECf;
			}
		}

		frontNgb->candidate = true;
		mCandidates.push(frontNgb);
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



void Grid::UpdateSimObjectLocation(const unsigned int inSimObjectID) {
	const vec3f& worldPos = mCOH->GetSimObjectPosition(inSimObjectID);
	const vec3f& worldDir = mCOH->GetSimObjectDirection(inSimObjectID);

	const std::vector<Cell      >& frontCells = mBuffers[mFrontBufferIdx].cells;
	const std::vector<Cell::Edge>& frontEdges = mBuffers[mFrontBufferIdx].edges;

	const unsigned int worldCellIdx = World2Cell(worldPos);
	const Cell* worldCell = &frontCells[worldCellIdx];
	const vec3f& worldVel = worldCell->GetInterpolatedVelocity(frontEdges, worldDir);

	if (std::isnan(worldVel.x) || std::isnan(worldVel.y) || std::isnan(worldVel.z)) { return; }
	if (std::isinf(worldVel.x) || std::isinf(worldVel.y) || std::isinf(worldVel.z)) { return; }

	if (worldVel.sqLen3D() > 0.01f) {
		mCOH->SetSimObjectRawPosition(inSimObjectID, worldPos + worldVel);
		mCOH->SetSimObjectRawDirection(inSimObjectID, worldVel.norm());
	}
}

void Grid::Reset() {
	// at the start of every frame, restore both grid buffers (cells
	// and edges) to the blank initial-state again from the backups
	// made in Init to undo the dynamic global (and per-group) data
	// write-operations
	numResets += 1;
	mBuffers[mFrontBufferIdx].cells.assign(mInitCells.begin(), mInitCells.end());
	mBuffers[mFrontBufferIdx].edges.assign(mInitEdges.begin(), mInitEdges.end());
	mBuffers[mBackBufferIdx].cells.assign(mInitCells.begin(), mInitCells.end());
	mBuffers[mBackBufferIdx].edges.assign(mInitEdges.begin(), mInitEdges.end());
	mTouchedCells.clear();

	mMaxDensity = -std::numeric_limits<float>::max();
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
		i = DIRECTION_EAST;
		a = dir.x;
	} else {
		i = DIRECTION_WEST;
		a = -dir.x;
	}

	if (dir.z >= 0.0f) {
		j = DIRECTION_NORTH;
		b = dir.z;
	} else {
		j = DIRECTION_SOUTH;
		b = -dir.z;
	}

	vel += (gridEdges[ edges[i] ].velocity * a);
	vel += (gridEdges[ edges[j] ].velocity * b);

	return vel;
}



// convert a world-space position to <x, y> grid-indices
vec3i Grid::World2Grid(const vec3f& inWorldPos) const {
	const int gx = (inWorldPos.x / mSquareSize);
	const int gz = (inWorldPos.z / mSquareSize);
	const int cx = std::max(0, std::min(int(mWidth - 1), gx));
	const int cz = std::max(0, std::min(int(mHeight - 1), gz));
	return vec3i(cx, 0, cz);
}

// get the 1D grid-cell index corresponding to a world-space position
unsigned int Grid::World2Cell(const vec3f& inWorldPos) const {
	const vec3i& gridPos = World2Grid(inWorldPos);
	const unsigned int gridIdx = GRID_INDEX(gridPos.x, gridPos.z);

	PFFG_ASSERT_MSG(gridIdx < mCells.size(), "world(%2.2f, %2.2f) grid(%d, %d)", inWorldPos.x, inWorldPos.z, gridPos.x, gridPos.z);
	return gridIdx;
}

// convert a cell's <x, y> grid-indices to world-space coordinates
vec3f Grid::Grid2World(const Cell* inCell) const {
	const float wx = inCell->x * mSquareSize;
	const float wz = inCell->y * mSquareSize;
	return vec3f(wx, ELEVATION(inCell->x, inCell->y), wz);
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

	for (unsigned int dir = 0; dir < NUM_DIRECTIONS; dir++) {
		speed[dir] = 0.0f;
		cost[dir]  = 0.0f;
	}
}
