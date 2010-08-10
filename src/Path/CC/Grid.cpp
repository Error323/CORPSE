#include <cmath>
#include <algorithm>
#include <limits>

#include "./Grid.hpp"
#include "../../System/Debugger.hpp"
#include "../../Sim/SimObjectDef.hpp"

#define GRID_ID(x,y) (((y) * (mWidth)) + (x))
#define ELEVATION(x,y) (mCOH->GetCenterHeightMap()[(mDownScale * (y)) * (mDownScale * mWidth) + (mDownScale * (x))])

const float Grid::sLambda     = 2.0f;
const float Grid::sMinDensity = 1.0f / pow(2.0f, sLambda);

void Grid::Init(const int inDownScale, ICallOutHandler* inCOH) {
	PFFG_ASSERT(inDownScale >= 1);

	mDownScale  = inDownScale;
	mCOH        = inCOH;
	mWidth      = mCOH->GetHeightMapSizeX() / mDownScale;
	mHeight     = mCOH->GetHeightMapSizeZ() / mDownScale;
	mSquareSize = mCOH->GetSquareSize()     * mDownScale;

	// NOTE: if mDownScale != 1, the engine's height-map must be downsampled
	printf("[Grid::Init] GridRes: %dx%d %d\n", mWidth, mHeight, mSquareSize);


	const unsigned int numCells = mWidth * mHeight;
	const unsigned int numEdges = (mWidth + 1) * mHeight + (mHeight + 1) * mWidth;

	mHeightVisData.resize(numCells);
	mSpeedVisData.resize(numCells);
	mCostVisData.resize(numCells);
	mPotentialVisData.resize(numCells);
	mDensityVisData.resize(numCells);
	mAvgVelocityVisData.resize(numCells);

	mEdges.reserve(numEdges);
	mEdgesBackup.reserve(numEdges);
	mCells.reserve(numCells);
	mCellsBackup.reserve(numCells);

	for (int y = 0; y < mHeight; y++) {
		for (int x = 0; x < mWidth; x++) {
			mCells.push_back(Cell(x,y));
			Cell* curCell = &mCells.back();

			Cell::Edge* edgeW = CreateEdge();
			Cell::Edge* edgeN = CreateEdge();

			curCell->edges[DIRECTION_WEST]  = edgeW;
			curCell->edges[DIRECTION_NORTH] = edgeN;

			unsigned int idx = 0;

			// Bind the east face of the cell west of the current cell
			if (x > 0) {
				idx = GRID_ID(x - 1, y);
				PFFG_ASSERT(idx < mCells.size());

				Cell* westCell = &mCells[idx];
				westCell->edges[DIRECTION_EAST] = edgeW;
			}

			// Bind the south face of the cell north of the current cell
			if (y > 0) {
				idx = GRID_ID(x, y - 1);
				PFFG_ASSERT(idx < mCells.size());

				Cell* northCell = &mCells[idx];
				northCell->edges[DIRECTION_SOUTH] = edgeN;
			}

			// Bind a new face to the southern face of the border cell
			if (y == mHeight - 1) {
				curCell->edges[DIRECTION_SOUTH] = CreateEdge();
			}

			// Bind a new face to the eastern face of the border cell
			if (x == mWidth - 1) {
				curCell->edges[DIRECTION_EAST] = CreateEdge();
			}
		}
	}

	PFFG_ASSERT(mCells.size() == numCells);
	PFFG_ASSERT(mEdges.size() == numEdges);

	// Perform a full reset on the cells and compute the height
	for (int y = 0; y < mHeight; y++) {
		for (int x = 0; x < mWidth; x++) {
			Cell* curCell = &mCells[GRID_ID(x, y)];

			// Full reset (sets potential to +inf, etc.)
			curCell->ResetFull();

			// Set the height, assuming the world is static wrt height
			curCell->height = ELEVATION(x, y);

			mHeightVisData[GRID_ID(x, y)] = curCell->height;
			mPotentialVisData[GRID_ID(x, y)] = curCell->potential;
		}
	}

	// Compute gradient-heights and neighbours
	for (int y = 0; y < mHeight; y++) {
		for (int x = 0; x < mWidth; x++) {
			Cell* cell = &mCells[GRID_ID(x, y)];

			if (y > 0) {
				cell->edges[DIRECTION_NORTH]->gradHeight = 
					vec3f(-mSquareSize, 0.0f, cell->height - mCells[GRID_ID(x, y - 1)].height);

				cell->neighbours[cell->numNeighbours++] = &mCells[GRID_ID(x, y - 1)];
			}

			if (y < mHeight - 1) {
				cell->edges[DIRECTION_SOUTH]->gradHeight = 
					vec3f( mSquareSize, 0.0f, cell->height - mCells[GRID_ID(x, y + 1)].height);

				cell->neighbours[cell->numNeighbours++] = &mCells[GRID_ID(x, y + 1)];
			}

			if (x > 0) {
				cell->edges[DIRECTION_WEST]->gradHeight = 
					vec3f(cell->height - mCells[GRID_ID(x-1, y)].height, 0.0f, -mSquareSize);

				cell->neighbours[cell->numNeighbours++] = &mCells[GRID_ID(x - 1, y)];
			}

			if (x < mWidth - 1) {
				cell->edges[DIRECTION_EAST]->gradHeight = 
					vec3f(cell->height - mCells[GRID_ID(x+1, y)].height, 0.0f, mSquareSize);

				cell->neighbours[cell->numNeighbours++] = &mCells[GRID_ID(x + 1, y)];
			}
		}
	}
	
	// Make a backup
	mCellsBackup.assign(mCells.begin(), mCells.end());
	mEdgesBackup.assign(mEdges.begin(), mEdges.end());
}

void Grid::AddDensityAndVelocity(const vec3f& inPos, const vec3f& inVel) {
	const vec3f posf = vec3f(inPos.x / mSquareSize, 0.0f, inPos.z / mSquareSize);
	const vec3i posi = World2Grid(inPos);

	const int i = (posf.x > posi.x + 0.5f) ? posi.x + 1 : posi.x;
	const int j = (posf.z > posi.z + 0.5f) ? posi.z + 1 : posi.z;

	PFFG_ASSERT(i > 0 && j > 0 && i < mWidth && j < mHeight);

	Cell* A = &mCells[GRID_ID(i - 1, j - 1)]; mTouchedCells[GRID_ID(i - 1, j - 1)] = A; 
	Cell* B = &mCells[GRID_ID(i    , j - 1)]; mTouchedCells[GRID_ID(i    , j - 1)] = B;
	Cell* C = &mCells[GRID_ID(i    , j    )]; mTouchedCells[GRID_ID(i    , j    )] = C;
	Cell* D = &mCells[GRID_ID(i - 1, j    )]; mTouchedCells[GRID_ID(i - 1, j    )] = D;

	// Add velocity
	C->avgVelocity += inVel;

	// Compute delta-X and delta-Y
	const float dX = posf.x - A->x + 0.5f;
	const float dY = posf.z - A->y + 0.5f;

	// Splat density
	A->density += pow(std::min<float>(1.0f - dX, 1.0f - dY), sLambda);
	B->density += pow(std::min<float>(       dX, 1.0f - dY), sLambda);
	C->density += pow(std::min<float>(       dX,        dY), sLambda);
	D->density += pow(std::min<float>(1.0f - dX,        dY), sLambda);
}

void Grid::ComputeAvgVelocity() {
	std::map<unsigned int, Cell*>::iterator i;

	for (i = mTouchedCells.begin(); i != mTouchedCells.end(); i++) {
		i->second->avgVelocity        /= i->second->density;
		mDensityVisData[i->first]      = i->second->density;
		mAvgVelocityVisData[i->first]  = i->second->avgVelocity;
	}
}

void Grid::ComputeSpeedAndUnitCost(Cell* cell) {
	const static vec3f dirVectors[] = {
		vec3f(  0.0f, 0.0f,  1.0f), // NORTH
		vec3f(  1.0f, 0.0f,  0.0f), // EAST
		vec3f(  0.0f, 0.0f, -1.0f), // SOUTH
		vec3f( -1.0f, 0.0f,  0.0f)  // WEST
	};

	const static float speedWeight      = 1.0f;
	const static float discomfortWeight = 100.0f;
	const static float maxDensity       = 10.0f;  // According to the Stetson-Harrison method
	const static float minSpeed         = 0.0f;

	cell->ResetGroupVars();

	const unsigned int cellIdx = GRID_ID(cell->x, cell->y);
	const vec3f& cellWorldPos = Grid2World(cell);

	for (unsigned int dir = 0; dir < NUM_DIRECTIONS; dir++) {
		const vec3i ngbGridPos = World2Grid(cellWorldPos + dirVectors[dir] * mMaxRadius);
		const unsigned int ngbIdx = GRID_ID(ngbGridPos.x, ngbGridPos.z);

		PFFG_ASSERT(ngbIdx < mCells.size());
		Cell* ngbCell = &mCells[ngbIdx];

		// Compute the speed- and unit-cost fields
		// TODO:
		//    evaluate speed and discomfort at cell into which
		//   an agent would move if it chose direction <dir>
		// FIXME: engine slopes should be in the same format as CC slopes
		const float topologicalSpeed = 
			mMaxSpeed +
			((cell->edges[dir]->gradHeight.dot2D(dirVectors[dir]) - mMinSlope) /  (mMaxSlope - mMinSlope)) * 
			(mMaxSpeed - minSpeed);

		const float flowSpeed = 
			ngbCell->avgVelocity.dot2D(dirVectors[dir]);

		const float speed = 
			((ngbCell->density - sMinDensity) / (maxDensity - sMinDensity)) * 
			(topologicalSpeed - flowSpeed) + 
			topologicalSpeed;
		const float cost = (speedWeight * speed + discomfortWeight * cell->discomfort) / speed;

		cell->speed[dir] = speed;
		cell->cost[dir] = cost;

		// FIXME: need a better way to represent the anisotropy
		mSpeedVisData[cellIdx] += speed;
		mCostVisData[cellIdx] += cost;
	}

	mSpeedVisData[cellIdx] /= NUM_DIRECTIONS;
	mCostVisData[cellIdx] /= NUM_DIRECTIONS;
}

void Grid::UpdateGroupPotentialField(const std::vector<Cell*>& inGoalCells, const std::set<unsigned int>& inSimObjectIds) {
	PFFG_ASSERT(!inGoalCells.empty());
	PFFG_ASSERT(mCandidates.empty());

	mMinSlope  =  std::numeric_limits<float>::max();
	mMaxSlope  = -std::numeric_limits<float>::max();
	mMaxSpeed  = -std::numeric_limits<float>::max();
	mMaxRadius = -std::numeric_limits<float>::max();

	for (std::set<unsigned int>::iterator i = inSimObjectIds.begin(); i != inSimObjectIds.end(); i++) {
		const SimObjectDef* simObjectDef = mCOH->GetSimObjectDef(*i);

		mMinSlope  = std::min<float>(mMinSlope,  simObjectDef->GetMinSlopeAngleCosine());
		mMaxSlope  = std::max<float>(mMaxSlope,  simObjectDef->GetMaxSlopeAngleCosine());
		mMaxSpeed  = std::max<float>(mMaxSpeed,  simObjectDef->GetMaxForwardSpeed());
		mMaxRadius = std::max<float>(mMaxRadius, mCOH->GetSimObjectRadius(*i));
	}

	// Add goal cells to the known set and add their neighbours to the
	// candidate-set
	for (size_t i = 0; i < inGoalCells.size(); i++) {
		Cell* cell = inGoalCells[i];
		ComputeSpeedAndUnitCost(cell);
		cell->known = true;
		cell->candidate = true;
		cell->potential = 0.0f;
		mPotentialVisData[GRID_ID(cell->x, cell->y)] = cell->potential;
		UpdateCandidates(cell);
	}

	while (!mCandidates.empty()) {
		Cell* cell = mCandidates.top(); mCandidates.pop();
		cell->known = true;
		mPotentialVisData[GRID_ID(cell->x, cell->y)] = cell->potential;
		UpdateCandidates(cell);
	}
}

void Grid::UpdateCandidates(const Cell* inParent) {
	for (int i = 0; i < inParent->numNeighbours; i++) {
		Cell* neighbour = inParent->neighbours[i];

		if (neighbour->known || neighbour->candidate) {
			continue;
		}

		ComputeSpeedAndUnitCost(neighbour);

		const int x = neighbour->x;
		const int y = neighbour->y;

		float dirCosts[NUM_DIRECTIONS] = {0.0f};
		Cell* dirCells[NUM_DIRECTIONS] = {NULL};
		bool  dirValid[NUM_DIRECTIONS] = {false};

		dirCells[DIRECTION_NORTH] = (y > 0        ) ? &mCells[GRID_ID(x    , y - 1)] : NULL;
		dirCells[DIRECTION_SOUTH] = (y < mHeight-1) ? &mCells[GRID_ID(x    , y + 1)] : NULL;
		dirCells[DIRECTION_WEST]  = (x > 0        ) ? &mCells[GRID_ID(x - 1, y    )] : NULL;
		dirCells[DIRECTION_EAST]  = (x < mWidth-1 ) ? &mCells[GRID_ID(x + 1, y    )] : NULL;

		for (unsigned int dir = 0; dir < NUM_DIRECTIONS; dir++) {
			if (dirCells[dir] == NULL) {
				dirValid[dir] = false;
				dirCosts[dir] = std::numeric_limits<float>::infinity();
			} else {
				dirCosts[dir] = dirCells[dir]->potential + dirCells[dir]->cost[dir];
				dirValid[dir] = dirCosts[dir] != std::numeric_limits<float>::infinity();
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

			Cell* bestX = NULL;
			Cell* bestY = NULL;
			int   bestDirX = -1;
			int   bestDirY = -1;

			if (dirCosts[DIRECTION_EAST] < dirCosts[DIRECTION_WEST]) {
				bestDirX = DIRECTION_EAST;
				bestX = dirCells[DIRECTION_EAST];
			} else {
				bestDirX = DIRECTION_WEST;
				bestX = dirCells[DIRECTION_WEST];
			}

			if (dirCosts[DIRECTION_NORTH] < dirCosts[DIRECTION_SOUTH]) {
				bestDirY = DIRECTION_NORTH;
				bestY = dirCells[DIRECTION_NORTH];
			} else {
				bestDirY = DIRECTION_SOUTH;
				bestY = dirCells[DIRECTION_SOUTH];
			}

			neighbour->potential = Potential2D(
				bestX->potential, neighbour->cost[bestDirX], 
				bestY->potential, neighbour->cost[bestDirY]
			);
			neighbour->edges[bestDirX]->gradPotential = vec3f(mSquareSize, 0.0f, neighbour->potential - bestX->potential);
			neighbour->edges[bestDirY]->gradPotential = vec3f(neighbour->potential - bestY->potential, 0.0f, mSquareSize);
			neighbour->edges[bestDirX]->gradPotential.x *= (bestDirX == DIRECTION_EAST) ? -1.0f : 1.0f;
			neighbour->edges[bestDirY]->gradPotential.y *= (bestDirY == DIRECTION_NORTH)? -1.0f : 1.0f;
		} else {
			if (undefinedX) {
				PFFG_ASSERT(dirValid[DIRECTION_NORTH] || dirValid[DIRECTION_SOUTH]);
				PFFG_ASSERT(!undefinedY);

				Cell* bestY = NULL;
				int   bestDirY = -1;

				if (dirCosts[DIRECTION_NORTH] < dirCosts[DIRECTION_SOUTH]) {
					bestDirY = DIRECTION_NORTH;
					bestY = dirCells[DIRECTION_NORTH];
				} else {
					bestDirY = DIRECTION_SOUTH;
					bestY = dirCells[DIRECTION_SOUTH];
				}

				neighbour->potential = Potential1D(bestY->potential, neighbour->cost[bestDirY]);
				neighbour->edges[bestDirY]->gradPotential = vec3f(mSquareSize, 0.0f, neighbour->potential - bestY->potential);
				neighbour->edges[bestDirY]->gradPotential.y *= (bestDirY == DIRECTION_NORTH)? -1.0f : 1.0f;
			}

			if (undefinedY) {
				PFFG_ASSERT(dirValid[DIRECTION_EAST] || dirValid[DIRECTION_WEST]);
				PFFG_ASSERT(!undefinedX);

				Cell* bestX = NULL;
				int   bestDirX = -1;

				if (dirCosts[DIRECTION_EAST] < dirCosts[DIRECTION_WEST]) {
					bestDirX = DIRECTION_EAST;
					bestX = dirCells[DIRECTION_EAST];
				} else {
					bestDirX = DIRECTION_WEST;
					bestX = dirCells[DIRECTION_WEST];
				}

				neighbour->potential = Potential1D(bestX->potential, neighbour->cost[bestDirX]);
				neighbour->edges[bestDirX]->gradPotential = vec3f(mSquareSize, 0.0f, neighbour->potential - bestX->potential);
				neighbour->edges[bestDirX]->gradPotential.x *= (bestDirX == DIRECTION_WEST)? -1.0f : 1.0f;
			}
		}

		neighbour->candidate = true;
		mCandidates.push(neighbour);
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

	const Cell* worldCell = World2Cell(worldPos);
	const vec3f& worldVel = worldCell->GetInterpolatedVelocity(worldDir);

	mCOH->SetSimObjectRawPosition(inSimObjectID, worldPos + worldVel);
	mCOH->SetSimObjectRawDirection(inSimObjectID, worldVel.norm());
}

void Grid::Reset() {
	numResets += 1;
	mCells.assign(mCellsBackup.begin(), mCellsBackup.end());
	mEdges.assign(mEdgesBackup.begin(), mEdgesBackup.end());
	mTouchedCells.clear();
}

Grid::Cell::Edge* Grid::CreateEdge() {
	mEdges.push_back(Grid::Cell::Edge());
	return &mEdges.back();
}

vec3f Grid::Cell::GetInterpolatedVelocity(const vec3f& dir) const {
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

	vel += (GetNormalizedPotentialGradient(i) * -speed[i]) * a;
	vel += (GetNormalizedPotentialGradient(j) * -speed[j]) * b;

	return vel;
}



// convert a world-space position to <x, y> grid-indices
vec3i Grid::World2Grid(const vec3f& inWorldPos) const {
	const int gx = int(inWorldPos.x / mSquareSize);
	const int gz = int(inWorldPos.z / mSquareSize);
	const int cx = std::max(0, std::min(mWidth - 1, gx));
	const int cz = std::max(0, std::min(mHeight - 1, gz));
	return vec3i(cx, 0, cz);
}

// get the grid-cell corresponding to a world-space position
Grid::Cell* Grid::World2Cell(const vec3f& inWorldPos) {
	const vec3i& gridPos = World2Grid(inWorldPos);
	const unsigned int gridIdx = GRID_ID(gridPos.x, gridPos.z);

	PFFG_ASSERT_MSG(gridIdx < mCells.size(), "world(%2.2f, %2.2f) grid(%d, %d)", inWorldPos.x, inWorldPos.z, gridPos.x, gridPos.z);

	return &mCells[gridIdx];
}

// convert a cell's <x, y> grid-indices to world-space coordinates
vec3f Grid::Grid2World(const Cell* inCell) const {
	const float wx = inCell->x * mSquareSize;
	const float wz = inCell->y * mSquareSize;
	return vec3f(wx, ELEVATION(inCell->x, inCell->y), wz);
}



Grid::~Grid() {
	mHeightVisData.clear();
	mSpeedVisData.clear();
	mCostVisData.clear();
	mPotentialVisData.clear();
	mDensityVisData.clear();
	mAvgVelocityVisData.clear();

	mTouchedCells.clear();
	mCells.clear();
	mEdges.clear();
}




void Grid::Cell::ResetFull() {
	ResetDynamicVars();

	height = 0.0f;
	numNeighbours = 0;

	for (unsigned int dir = 0; dir < NUM_DIRECTIONS; dir++) {
		edges[dir]->gradHeight = NVECf;
		neighbours[dir] = NULL;
	}
}

void Grid::Cell::ResetDynamicVars() {
	ResetGroupVars();

	avgVelocity = NVECf;
	density     = 0.0f;
	discomfort  = 0.0f;
}

void Grid::Cell::ResetGroupVars() {
	potential = std::numeric_limits<float>::infinity();
	known = false;
	candidate = false;

	for (unsigned int dir = 0; dir < NUM_DIRECTIONS; dir++) {
		speed[dir] = 0.0f;
		cost[dir]  = 0.0f;
		edges[dir]->gradPotential = NVECf;
		edges[dir]->velocity = NVECf;
	}
}
