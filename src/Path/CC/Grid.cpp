#include <cmath>
#include <algorithm>
#include <limits>

#include "./Grid.hpp"
#include "../../System/Debugger.hpp"
#include "../../Sim/SimObjectDef.hpp"

#define GRID_ID(x,y) (((y)*(mWidth))+(x))
#define ELEVATION(x,y) (mCoh->GetCenterHeightMap()[(mDownScale*(y))*(mDownScale*mWidth)+(mDownScale*(x))])

const float Grid::sLambda     = 2.0f;
const float Grid::sMinDensity = 1.0f / pow(2.0f, sLambda);

void Grid::Init(const int inDownScale, ICallOutHandler* inCoh) {
	PFFG_ASSERT(inDownScale >= 1);

	mDownScale  = inDownScale;
	mCoh        = inCoh;
	mWidth      = mCoh->GetHeightMapSizeX() / mDownScale;
	mHeight     = mCoh->GetHeightMapSizeZ() / mDownScale;
	mSquareSize = mCoh->GetSquareSize()     * mDownScale;

	//! NOTE: if mDownScale != 1, the engine's height-map must be downsampled
	printf("[Grid::Init] GridRes: %dx%d %d\n", mWidth, mHeight, mSquareSize);


	const unsigned int numCells = mWidth * mHeight;
	const unsigned int numEdges = (mWidth + 1) * mHeight + (mHeight + 1) * mWidth;

	mHeightData.reserve(numCells);
	mDensityData.reserve(numCells);
	mVelocityData.reserve(numCells);

	mEdges.reserve(numEdges);
	mCells.reserve(numCells);

	for (int y = 0; y < mHeight; y++) {
		for (int x = 0; x < mWidth; x++) {
			Cell* curCell = new Cell(x, y);
			mCells.push_back(curCell);

			Cell::Edge* horizontal = CreateEdge();
			Cell::Edge* vertical   = CreateEdge();

			curCell->edges[DIRECTION_WEST]  = horizontal;
			curCell->edges[DIRECTION_NORTH] = vertical;

			// Bind the east face of the cell west of the current cell
			if (x > 0) {
				Cell* westCell = mCells[GRID_ID(x - 1, y)];
				westCell->edges[DIRECTION_EAST] = horizontal;
			}

			// Bind the south face of the cell north of the current cell
			if (y > 0) {
				Cell* northCell = mCells[GRID_ID(x, y - 1)];
				northCell->edges[DIRECTION_SOUTH] = vertical;
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

	// Perform a full reset on the cells and compute the height
	for (int y = 0; y < mHeight; y++) {
		for (int x = 0; x < mWidth; x++) {
			Cell* curCell = mCells[GRID_ID(x,y)];

			// Full reset
			curCell->ResetFull();

			// Set the height, assuming the world is static wrt height
			curCell->height = ELEVATION(x, y);
			mHeightData[GRID_ID(x, y)] = curCell->height;
		}
	}

	// Compute gradient-heights and neighbours
	for (int y = 0; y < mHeight; y++) {
		for (int x = 0; x < mWidth; x++) {
			Cell* cell = mCells[GRID_ID(x, y)];

			if (y > 0) {
				cell->edges[DIRECTION_NORTH]->gradHeight = 
					vec3f(-mSquareSize, 0.0f, cell->height - mCells[GRID_ID(x, y-1)]->height);

				cell->neighbours[cell->numNeighbours++] = mCells[GRID_ID(x, y-1)];
			}

			if (y < mHeight - 1) {
				cell->edges[DIRECTION_SOUTH]->gradHeight = 
					vec3f( mSquareSize, 0.0f, cell->height - mCells[GRID_ID(x, y+1)]->height);

				cell->neighbours[cell->numNeighbours++] = mCells[GRID_ID(x, y+1)];
			}

			if (x > 0) {
				cell->edges[DIRECTION_WEST]->gradHeight = 
					vec3f(cell->height - mCells[GRID_ID(x-1, y)]->height, 0.0f, -mSquareSize);

				cell->neighbours[cell->numNeighbours++] = mCells[GRID_ID(x-1, y)];
			}

			if (x < mWidth - 1) {
				cell->edges[DIRECTION_EAST]->gradHeight = 
					vec3f(cell->height - mCells[GRID_ID(x+1, y)]->height, 0.0f, mSquareSize);

				cell->neighbours[cell->numNeighbours++] = mCells[GRID_ID(x+1, y)];
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

void Grid::ComputeSpeedFieldAndUnitCost(const std::set<unsigned int>& inSimObjectIds) {
	const static vec3f dirVectors[] = {
		vec3f(  0.0f, 0.0f,  1.0f), // NORTH
		vec3f(  1.0f, 0.0f,  0.0f), // EAST
		vec3f(  0.0f, 0.0f, -1.0f), // SOUTH
		vec3f( -1.0f, 0.0f,  0.0f)  // WEST
	};

	const static float speedWeight      = 1.0f;   // No extra weight
	const static float discomfortWeight = 100.0f; // Discomfort means YOU SHALL NOT PASS
	const static float maxDensity       = 10.0f;  // According to the Stetson-Harrison method
	const static float minSpeed         = 0.0f;

	             float minSlope         = std::numeric_limits<float>::max();
	             float maxSlope         = std::numeric_limits<float>::min();
	             float maxSpeed         = std::numeric_limits<float>::min();
	             float maxRadius        = std::numeric_limits<float>::min();

	for (std::set<unsigned int>::iterator i = inSimObjectIds.begin(); i != inSimObjectIds.end(); i++) {
		const SimObjectDef* simObjectDef = mCoh->GetSimObjectDef(*i);

		minSlope  = std::min<float>(minSlope,  simObjectDef->GetMinSlopeAngleCosine());
		maxSlope  = std::max<float>(maxSlope,  simObjectDef->GetMaxSlopeAngleCosine());
		maxSpeed  = std::max<float>(maxSpeed,  simObjectDef->GetMaxForwardSpeed());
		maxRadius = std::max<float>(maxRadius, mCoh->GetSimObjectRadius(*i));
	}

	for (int i = 0, n = mWidth*mHeight; i < n; i++) {
		Cell* cell = mCells[i];
		cell->ResetGroupVars();
		
		const vec3f worldPos = Grid2World(cell);
		for (int dir = 0; dir < NUM_DIRECTIONS; dir++) {
			// Compute the speed field
			vec3i dGridPos = World2Grid(worldPos + dirVectors[dir]*maxRadius);
			Cell* dCell = mCells[GRID_ID(dGridPos.x, dGridPos.z)];

			// FIXME: Engine slope should be in the same format as CC slope
			float topologicalSpeed = 
				maxSpeed + ((cell->edges[dir]->gradHeight.dot2D(dirVectors[dir]) - minSlope) / 
				(maxSlope - minSlope)) * 
				(maxSpeed - minSpeed);

			float flowSpeed = 
				dCell->avgVelocity.dot2D(dirVectors[dir]);
			
			float speed = 
				((dCell->density - sMinDensity) / (maxDensity - sMinDensity)) * 
				(topologicalSpeed - flowSpeed) + 
				topologicalSpeed;

			cell->speed[dir] = speed;

			// Compute the unit cost
			cell->cost[dir] = (speedWeight*speed + discomfortWeight*cell->discomfort) / speed;
		}
	}
}

void Grid::UpdateGroupPotentialField(const std::vector<Cell*>& inGoalCells) {
	PFFG_ASSERT(!inGoalCells.empty());
	PFFG_ASSERT(mCandidates.empty());

	// Initialize the known and unknown set
	for (size_t i = 0; i < mCells.size(); i++) {
		if (std::find(inGoalCells.begin(), inGoalCells.end(), mCells[i]) == inGoalCells.end()) {
			mCells[i]->potential = std::numeric_limits<float>::max();
			mCells[i]->known = false;
		}
		else {
			mCells[i]->potential = 0.0f;
			mCells[i]->known = true;
			UpdateCandidates(mCells[i]);
		}
	}

	while (!mCandidates.empty()) {
		Cell* cell = mCandidates.top(); mCandidates.pop();
		cell->known = true;
		UpdateCandidates(cell);
	}
}

void Grid::UpdateCandidates(const Cell* inParent) {
	static const float maxf = std::numeric_limits<float>::max();
	for (int i = 0; i < inParent->numNeighbours; i++) {
		Cell* neighbour = inParent->neighbours[i];
		if (neighbour->known)
			continue;

		float dirCosts[NUM_DIRECTIONS];
		Cell* dirCells[NUM_DIRECTIONS];
		dirCells[DIRECTION_NORTH] = mCells[GRID_ID(neighbour->x, neighbour->y - 1)];
		dirCells[DIRECTION_SOUTH] = mCells[GRID_ID(neighbour->x, neighbour->y + 1)];
		dirCells[DIRECTION_EAST]  = mCells[GRID_ID(neighbour->x + 1, neighbour->y)];
		dirCells[DIRECTION_WEST]  = mCells[GRID_ID(neighbour->x - 1, neighbour->y)];

		dirCosts[DIRECTION_NORTH] = dirCells[DIRECTION_NORTH]->potential + 
			dirCells[DIRECTION_NORTH]->cost[DIRECTION_NORTH];
		dirCosts[DIRECTION_SOUTH] = dirCells[DIRECTION_SOUTH]->potential + 
			dirCells[DIRECTION_SOUTH]->cost[DIRECTION_SOUTH];
		dirCosts[DIRECTION_EAST]  = dirCells[DIRECTION_EAST ]->potential + 
			dirCells[DIRECTION_EAST ]->cost[DIRECTION_EAST ];
		dirCosts[DIRECTION_WEST]  = dirCells[DIRECTION_WEST ]->potential + 
			dirCells[DIRECTION_WEST ]->cost[DIRECTION_WEST ];

		// Potential undefined on the x-axis
		if (dirCosts[DIRECTION_WEST] >= maxf && dirCosts[DIRECTION_EAST] >= maxf) {
			Cell* bestY;
			int   bestDirY;

			if (dirCosts[DIRECTION_NORTH] < dirCosts[DIRECTION_SOUTH]) {
				bestDirY = DIRECTION_NORTH;
				bestY = dirCells[DIRECTION_NORTH];
			}
			else {
				bestDirY = DIRECTION_SOUTH;
				bestY = dirCells[DIRECTION_SOUTH];
			}

			neighbour->potential = Potential1D(bestY->potential, neighbour->cost[bestDirY]);
			neighbour->edges[bestDirY]->gradPotential = vec3f(mSquareSize, 0.0f, neighbour->potential - bestY->potential);
			neighbour->edges[bestDirY]->gradPotential.y *= (bestDirY == DIRECTION_NORTH)  ? -1.0f : 1.0f;
		}

		// Potential undefined on the y-axis
		else
		if (dirCosts[DIRECTION_NORTH] >= maxf && dirCosts[DIRECTION_SOUTH] >= maxf) {
			Cell* bestX;
			int   bestDirX;

			if (dirCosts[DIRECTION_EAST] < dirCosts[DIRECTION_WEST]) {
				bestDirX = DIRECTION_EAST;
				bestX = dirCells[DIRECTION_EAST];
			}
			else {
				bestDirX = DIRECTION_WEST;
				bestX = dirCells[DIRECTION_WEST];
			}

			neighbour->potential = Potential1D(bestX->potential, neighbour->cost[bestDirX]);
			neighbour->edges[bestDirX]->gradPotential = vec3f(mSquareSize, 0.0f, neighbour->potential - bestX->potential);
			neighbour->edges[bestDirX]->gradPotential.x *= (bestDirX == DIRECTION_EAST)  ? -1.0f : 1.0f;
		}

		// Potential defined on both axis
		else {
			Cell* bestX;
			Cell* bestY;
			int   bestDirX;
			int   bestDirY;

			if (dirCosts[DIRECTION_EAST] < dirCosts[DIRECTION_WEST]) {
				bestDirX = DIRECTION_EAST;
				bestX = dirCells[DIRECTION_EAST];
			}
			else {
				bestDirX = DIRECTION_WEST;
				bestX = dirCells[DIRECTION_WEST];
			}

			if (dirCosts[DIRECTION_NORTH] < dirCosts[DIRECTION_SOUTH]) {
				bestDirY = DIRECTION_NORTH;
				bestY = dirCells[DIRECTION_NORTH];
			}
			else {
				bestDirY = DIRECTION_SOUTH;
				bestY = dirCells[DIRECTION_SOUTH];
			}

			neighbour->potential = Potential2D(bestX->potential, neighbour->cost[bestDirX], bestY->potential, neighbour->cost[bestDirY]);
			neighbour->edges[bestDirX]->gradPotential = vec3f(mSquareSize, 0.0f, neighbour->potential - bestX->potential);
			neighbour->edges[bestDirY]->gradPotential = vec3f(neighbour->potential - bestY->potential, 0.0f, mSquareSize);
			neighbour->edges[bestDirX]->gradPotential.x *= (bestDirX == DIRECTION_EAST)  ? -1.0f : 1.0f;
			neighbour->edges[bestDirY]->gradPotential.y *= (bestDirY == DIRECTION_NORTH) ? -1.0f : 1.0f;
		}

		mCandidates.push(neighbour);
	}
}

float Grid::Potential1D(const float inPot, const float inCost) {
	return std::max<float>(inPot + inCost, inPot - inCost);
}

float Grid::Potential2D(const float inPotX, const float inCostX, const float inPotY, const float inCostY) {
	const float b = 2.0f*inPotX + 2.0f*inPotY;
	const float d = sqrt(-4.0f * inPotX*inPotX  +  4.0f * inPotY*inPotY  +
		8.0f * inCostX*inCostX * inCostY*inCostY);

	const float solution1 = (-b + d) / 4.0f;
	const float solution2 = (-b - d) / 4.0f;

	return std::max<float>(solution1, solution2);
}

void Grid::UpdateSimObjectLocation(const int inSimObjectId) {
}

void Grid::Reset() {
	std::map<unsigned int, Cell*>::iterator i;
	for (i = mTouchedCells.begin(); i != mTouchedCells.end(); i++)
		i->second->ResetDynamicVars();
	
	mTouchedCells.clear();
}

Cell::Edge* Grid::CreateEdge() {
	Cell::Edge* f = new Cell::Edge();
	mEdges.push_back(f);
	return f;
}

vec3i Grid::World2Grid(const vec3f& inVec) {
	const int x = std::max(0, std::min( mWidth - 1, int(round(inVec.x / mSquareSize)) ));
	const int z = std::max(0, std::min( mHeight - 1, int(round(inVec.z / mSquareSize)) ));
	return vec3i(x, 0, z);
}

vec3f Grid::Grid2World(const Cell* inCell) {
	return vec3f(inCell->x * mSquareSize, ELEVATION(inCell->x, inCell->y), inCell->y * mSquareSize);
}

Grid::~Grid() {
	for (size_t i = 0; i < mCells.size(); i++)
		delete mCells[i];

	for (size_t i = 0; i < mEdges.size(); i++)
		delete mEdges[i];
}




void Cell::ResetFull() {
	ResetDynamicVars();
	height = 0.0f;
	numNeighbours = 0;

	for (int dir = 0; dir < NUM_DIRECTIONS; dir++) {
		edges[dir]->gradHeight = NVECf;
		neighbours[dir] = NULL;
	}
}

void Cell::ResetDynamicVars() {
	ResetGroupVars();
	avgVelocity = NVECf;
	density     = 0.0f;
	discomfort = 0.0f;
}

void Cell::ResetGroupVars() {
	potential  = std::numeric_limits<float>::max();
	for (int dir = 0; dir < NUM_DIRECTIONS; dir++) {
		speed[dir] = 0.0f;
		cost[dir]  = 0.0f;
		edges[dir]->gradPotential = NVECf;
		edges[dir]->velocity = NVECf;
	}
}
