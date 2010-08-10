#include <cmath>
#include <algorithm>
#include <limits>

#include "./Grid.hpp"
#include "../../System/Debugger.hpp"
#include "../../Sim/SimObjectDef.hpp"

#define GRID_ID(x,y) (((y) * (mWidth)) + (x))
#define ELEVATION(x,y) (mCoh->GetCenterHeightMap()[(mDownScale * (y)) * (mDownScale * mWidth) + (mDownScale * (x))])

const float Grid::sLambda     = 2.0f;
const float Grid::sMinDensity = 1.0f / pow(2.0f, sLambda);

void Grid::Init(const int inDownScale, ICallOutHandler* inCoh) {
	PFFG_ASSERT(inDownScale >= 1);

	mDownScale  = inDownScale;
	mCoh        = inCoh;
	mWidth      = mCoh->GetHeightMapSizeX() / mDownScale;
	mHeight     = mCoh->GetHeightMapSizeZ() / mDownScale;
	mSquareSize = mCoh->GetSquareSize()     * mDownScale;

	// NOTE: if mDownScale != 1, the engine's height-map must be downsampled
	printf("[Grid::Init] GridRes: %dx%d %d\n", mWidth, mHeight, mSquareSize);


	const unsigned int numCells = mWidth * mHeight;
	const unsigned int numEdges = (mWidth + 1) * mHeight + (mHeight + 1) * mWidth;

	mHeightData.resize(numCells);
	mPotentialData.resize(numCells);
	mDensityData.resize(numCells);
	mVelocityData.resize(numCells);

	mEdges.reserve(numEdges);
	mCells.reserve(numCells);

	for (int y = 0; y < mHeight; y++) {
		for (int x = 0; x < mWidth; x++) {
			Cell* curCell = new Cell(x, y);
			mCells.push_back(curCell);

			Cell::Edge* edgeW = CreateEdge();
			Cell::Edge* edgeN = CreateEdge();

			curCell->edges[DIRECTION_WEST]  = edgeW;
			curCell->edges[DIRECTION_NORTH] = edgeN;

			unsigned int idx = 0;

			// Bind the east face of the cell west of the current cell
			if (x > 0) {
				idx = GRID_ID(x - 1, y);
				PFFG_ASSERT(idx < mCells.size());

				Cell* westCell = mCells[idx];
				westCell->edges[DIRECTION_EAST] = edgeW;
			}

			// Bind the south face of the cell north of the current cell
			if (y > 0) {
				idx = GRID_ID(x, y - 1);
				PFFG_ASSERT(idx < mCells.size());

				Cell* northCell = mCells[idx];
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
			Cell* curCell = mCells[GRID_ID(x, y)];

			// Full reset (sets potential to +inf, etc.)
			curCell->ResetFull();

			// Set the height, assuming the world is static wrt height
			curCell->height = ELEVATION(x, y);

			// NOTE:
			//    initializing the potentials to +inf makes
			//    the field invisible (due to normalization)
			mHeightData[GRID_ID(x, y)] = curCell->height;
			mPotentialData[GRID_ID(x, y)] = curCell->potential;
		}
	}

	// Compute gradient-heights and neighbours
	for (int y = 0; y < mHeight; y++) {
		for (int x = 0; x < mWidth; x++) {
			Cell* cell = mCells[GRID_ID(x, y)];

			if (y > 0) {
				cell->edges[DIRECTION_NORTH]->gradHeight = 
					vec3f(-mSquareSize, 0.0f, cell->height - mCells[GRID_ID(x, y - 1)]->height);

				cell->neighbours[cell->numNeighbours++] = mCells[GRID_ID(x, y - 1)];
			}

			if (y < mHeight - 1) {
				cell->edges[DIRECTION_SOUTH]->gradHeight = 
					vec3f( mSquareSize, 0.0f, cell->height - mCells[GRID_ID(x, y + 1)]->height);

				cell->neighbours[cell->numNeighbours++] = mCells[GRID_ID(x, y + 1)];
			}

			if (x > 0) {
				cell->edges[DIRECTION_WEST]->gradHeight = 
					vec3f(cell->height - mCells[GRID_ID(x-1, y)]->height, 0.0f, -mSquareSize);

				cell->neighbours[cell->numNeighbours++] = mCells[GRID_ID(x - 1, y)];
			}

			if (x < mWidth - 1) {
				cell->edges[DIRECTION_EAST]->gradHeight = 
					vec3f(cell->height - mCells[GRID_ID(x+1, y)]->height, 0.0f, mSquareSize);

				cell->neighbours[cell->numNeighbours++] = mCells[GRID_ID(x + 1, y)];
			}
		}
	}
}

void Grid::AddDensityAndVelocity(const vec3f& inPos, const vec3f& inVel) {
	const vec3f posf = vec3f(inPos.x / mSquareSize, 0.0f, inPos.z / mSquareSize);
	const vec3i posi = World2Grid(inPos);

	const int i = (posf.x > posi.x + 0.5f) ? posi.x + 1 : posi.x;
	const int j = (posf.z > posi.z + 0.5f) ? posi.z + 1 : posi.z;

	PFFG_ASSERT(i > 0 && j > 0 && i < mWidth && j < mHeight);

	Cell* A = mCells[GRID_ID(i - 1, j - 1)]; mTouchedCells[GRID_ID(i - 1, j - 1)] = A; 
	Cell* B = mCells[GRID_ID(i    , j - 1)]; mTouchedCells[GRID_ID(i    , j - 1)] = B;
	Cell* C = mCells[GRID_ID(i    , j    )]; mTouchedCells[GRID_ID(i    , j    )] = C;
	Cell* D = mCells[GRID_ID(i - 1, j    )]; mTouchedCells[GRID_ID(i - 1, j    )] = D;

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
		i->second->avgVelocity /= i->second->density;
		mDensityData[i->first]  = i->second->density;
		mVelocityData[i->first] = i->second->avgVelocity;
	}
}

void Grid::UpdateGroupPotentialField(const std::vector<Cell*>& inGoalCells, const std::set<unsigned int>& inSimObjectIds) {
	PFFG_ASSERT(!inGoalCells.empty());
	PFFG_ASSERT(mCandidates.empty());

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

	             float minSlope         =  std::numeric_limits<float>::max();
	             float maxSlope         = -std::numeric_limits<float>::max();
	             float maxSpeed         = -std::numeric_limits<float>::max();
	             float maxRadius        = -std::numeric_limits<float>::max();

	for (std::set<unsigned int>::iterator i = inSimObjectIds.begin(); i != inSimObjectIds.end(); i++) {
		const SimObjectDef* simObjectDef = mCoh->GetSimObjectDef(*i);

		minSlope  = std::min<float>(minSlope,  simObjectDef->GetMinSlopeAngleCosine());
		maxSlope  = std::max<float>(maxSlope,  simObjectDef->GetMaxSlopeAngleCosine());
		maxSpeed  = std::max<float>(maxSpeed,  simObjectDef->GetMaxForwardSpeed());
		maxRadius = std::max<float>(maxRadius, mCoh->GetSimObjectRadius(*i));
	}

	// This loop computes the speedfield, unitcost and initializes the known
	// and unknown set
	for (size_t i = 0; i < mCells.size(); i++) {
		// Compute the speedfield and unit cost
		Cell* cell = mCells[i];
		cell->ResetGroupVars();

		const vec3f& worldPos = Grid2World(cell);

		for (int dir = 0; dir < NUM_DIRECTIONS; dir++) {
			// Compute the speed field
			const vec3i dGridPos = World2Grid(worldPos + dirVectors[dir] * maxRadius);
			const unsigned int idx = GRID_ID(dGridPos.x, dGridPos.z);

			PFFG_ASSERT(idx < mCells.size());
			Cell* dCell = mCells[idx];

			// FIXME: Engine slope should be in the same format as CC slope
			const float topologicalSpeed = 
				maxSpeed + ((cell->edges[dir]->gradHeight.dot2D(dirVectors[dir]) - minSlope) / 
				(maxSlope - minSlope)) * 
				(maxSpeed - minSpeed);

			const float flowSpeed = 
				dCell->avgVelocity.dot2D(dirVectors[dir]);

			const float speed = 
				((dCell->density - sMinDensity) / (maxDensity - sMinDensity)) * 
				(topologicalSpeed - flowSpeed) + 
				topologicalSpeed;

			cell->speed[dir] = speed;

			// Compute the unit cost
			cell->cost[dir] = (speedWeight * speed + discomfortWeight * cell->discomfort) / speed;
		}

		// Initialize the known and unknown set
		if (std::find(inGoalCells.begin(), inGoalCells.end(), cell) == inGoalCells.end()) {
			cell->potential = std::numeric_limits<float>::infinity();
			cell->known = false;
		} else {
			cell->potential = 0.0f;
			cell->known = true;
			mCandidates.push(cell);
		}
		mPotentialData[i] = cell->potential;
	}

	while (!mCandidates.empty()) {
		Cell* cell = mCandidates.top(); mCandidates.pop();
		cell->known = true;
		UpdateCandidates(cell);
	}
}

void Grid::UpdateCandidates(const Cell* inParent) {
	for (int i = 0; i < inParent->numNeighbours; i++) {
		Cell* neighbour = inParent->neighbours[i];

		if (neighbour->known)
			continue;

		const int x = neighbour->x;
		const int y = neighbour->y;

		float dirCosts[NUM_DIRECTIONS] = {0.0f};
		Cell* dirCells[NUM_DIRECTIONS] = {NULL};
		bool  dirValid[NUM_DIRECTIONS] = {false};

		dirCells[DIRECTION_NORTH] = (y > 0        ) ? mCells[GRID_ID(x    , y - 1)] : NULL;
		dirCells[DIRECTION_SOUTH] = (y < mHeight-1) ? mCells[GRID_ID(x    , y + 1)] : NULL;
		dirCells[DIRECTION_WEST]  = (x > 0        ) ? mCells[GRID_ID(x - 1, y    )] : NULL;
		dirCells[DIRECTION_EAST]  = (x < mWidth-1 ) ? mCells[GRID_ID(x + 1, y    )] : NULL;

		for (int dir = 0; dir < NUM_DIRECTIONS; dir++) {
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
		if (undefinedX) {
			PFFG_ASSERT(dirValid[DIRECTION_NORTH] || dirValid[DIRECTION_SOUTH]);

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
			neighbour->edges[bestDirY]->gradPotential = 
				vec3f(mSquareSize, 0.0f, neighbour->potential - bestY->potential);
			neighbour->edges[bestDirY]->gradPotential.y *= 
				(bestDirY == DIRECTION_NORTH)  ? -1.0f : 1.0f;
		}

		// Potential undefined on the y-axis
		else if (undefinedY) {
			PFFG_ASSERT(dirValid[DIRECTION_EAST] || dirValid[DIRECTION_WEST]);
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
			neighbour->edges[bestDirX]->gradPotential = 
				vec3f(mSquareSize, 0.0f, neighbour->potential - bestX->potential);
			neighbour->edges[bestDirX]->gradPotential.x *= 
				(bestDirX == DIRECTION_WEST)  ? -1.0f : 1.0f;
		} else {
			// Potential defined on both axis
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

			const float abcform = Potential2DAbcform(bestX->potential, neighbour->cost[bestDirX], 
				bestY->potential, neighbour->cost[bestDirY]);
			const float wolfram = Potential2DWolfram(bestX->potential, neighbour->cost[bestDirX], 
				bestY->potential, neighbour->cost[bestDirY]);

			PFFG_ASSERT_MSG(fabs(abcform - wolfram) < 0.0001f, "wolfram(%f) != abcform(%f)", wolfram, abcform);
			neighbour->potential = abcform;
			neighbour->edges[bestDirX]->gradPotential = vec3f(mSquareSize, 0.0f, neighbour->potential - bestX->potential);
			neighbour->edges[bestDirY]->gradPotential = vec3f(neighbour->potential - bestY->potential, 0.0f, mSquareSize);
			neighbour->edges[bestDirX]->gradPotential.x *= (bestDirX == DIRECTION_EAST)  ? -1.0f : 1.0f;
			neighbour->edges[bestDirY]->gradPotential.y *= (bestDirY == DIRECTION_NORTH) ? -1.0f : 1.0f;
		}

		PFFG_ASSERT(neighbour->potential != std::numeric_limits<float>::infinity());
		//printf("C_nb(%d,%d) = %+2.2f\n", neighbour->x, neighbour->y, neighbour->potential);
		mPotentialData[GRID_ID(x, y)] = neighbour->potential;
		mCandidates.push(neighbour);
	}
}

inline
float Grid::Potential1D(const float inPot, const float inCost) {
	return std::max<float>(inPot + inCost, inPot - inCost);
}

inline
float Grid::Potential2DAbcform(const float p1, const float c1, const float p2, const float c2) {
	static const float a = 2.0f;
	       const float b = -(2.0f*p1 + 2.0f*p2);
	       const float c = (p1*p1 + p2*p2) - ((c1*c1) * (c2*c2));
	       const float d = b*b - 4.0f*a*c;
	PFFG_ASSERT(d > 0.0f);
	       const float e = sqrtf(d);

	       const float solution1 = (-b + e) / (2.0f*a);
	       const float solution2 = (+b + e) / (2.0f*a);

	return std::max<float>(solution1, solution2);
}

inline
float Grid::Potential2DWolfram(const float p1, const float c1, const float p2, const float c2) {
	// Wolfram solution to equation (11)
	// solve ( (x-p1) / c1 )^2  +  ( (x-p2) / c2 )^2 = 1 for x

	PFFG_ASSERT(c1 != 0.0f && c2 != 0.0f);

	const float c1s = c1*c1;
	const float c2s = c2*c2;
	const float p1_min_p2 = (p1 - p2);
	const float square = c1s*c2s * (c1s + c2s - p1_min_p2*p1_min_p2);

	PFFG_ASSERT(square > 0.0f);

	const float nom1 = sqrtf(square);
	const float nom2 = c1s*p2 + c2s*p1;
	const float denom = c1s + c2s;

	const float solution1 = (-nom1 + nom2) / denom;
	const float solution2 = ( nom1 + nom2) / denom;


	return std::max<float>(solution1, solution2);
}

void Grid::UpdateSimObjectLocation(const int inSimObjectId) {
}

void Grid::Reset() {
	numResets += 1;

	std::map<unsigned int, Cell*>::iterator i;
	for (i = mTouchedCells.begin(); i != mTouchedCells.end(); i++)
		i->second->ResetDynamicVars();

	mTouchedCells.clear();
}

Grid::Cell::Edge* Grid::CreateEdge() {
	Grid::Cell::Edge* f = new Grid::Cell::Edge();
	mEdges.push_back(f);
	return f;
}



vec3i Grid::World2Grid(const vec3f& inVec) {
	const int x = std::max(0, std::min( mWidth - 1, int(round(inVec.x / mSquareSize)) ));
	const int z = std::max(0, std::min( mHeight - 1, int(round(inVec.z / mSquareSize)) ));
	return vec3i(x, 0, z);
}

Grid::Cell* Grid::World2Cell(const vec3f& inPos) {
	const vec3i& cellCoords = World2Grid(inPos);
	const unsigned int idx = GRID_ID(cellCoords.x, cellCoords.z);

	PFFG_ASSERT(idx < mCells.size());
	return mCells[idx];
}

vec3f Grid::Grid2World(const Cell* inCell) {
	return vec3f(inCell->x * mSquareSize, ELEVATION(inCell->x, inCell->y), inCell->y * mSquareSize);
}



Grid::~Grid() {
	for (size_t i = 0; i < mCells.size(); i++)
		delete mCells[i];

	for (size_t i = 0; i < mEdges.size(); i++)
		delete mEdges[i];

	mHeightData.clear();
	mPotentialData.clear();
	mDensityData.clear();
	mVelocityData.clear();

	mTouchedCells.clear();
	mCells.clear();
	mEdges.clear();
}




void Grid::Cell::ResetFull() {
	ResetDynamicVars();

	height = 0.0f;
	numNeighbours = 0;

	for (int dir = 0; dir < NUM_DIRECTIONS; dir++) {
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

	for (int dir = 0; dir < NUM_DIRECTIONS; dir++) {
		speed[dir] = 0.0f;
		cost[dir]  = 0.0f;
		edges[dir]->gradPotential = NVECf;
		edges[dir]->velocity = NVECf;
	}
}
