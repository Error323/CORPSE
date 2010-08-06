#include <cmath>

#include "./Grid.hpp"
#include "../../System/Debugger.hpp"
#include "../../Sim/SimObjectDef.hpp"

#define GRID_ID(x,y) (((y)*(mWidth))+(x))
#define ELEVATION(x,y) (mCoh->GetCenterHeightMap()[(mDownScale*(y))*(mDownScale*mWidth)+(mDownScale*(x))])

void Grid::Init(const int inDownScale, ICallOutHandler* inCoh) {
	PFFG_ASSERT(inDownScale >= 1);

	mDownScale  = inDownScale;
	mCoh        = inCoh;
	mWidth      = mCoh->GetHeightMapSizeX() / mDownScale;
	mHeight     = mCoh->GetHeightMapSizeZ() / mDownScale;
	mSquareSize = mCoh->GetSquareSize()     * mDownScale;

	//! NOTE: if mDownScale != 1, the engine's height-map must be downsampled
	printf("[Grid::Init] GridRes: %dx%d %d\n", mWidth, mHeight, mSquareSize);

	mLambda = 2.0f;
	mMinDensity = 1.0f / pow(2.0f, mLambda);

	unsigned int cells = mWidth*mHeight;
	unsigned int faces = (mWidth+1)*mHeight + (mHeight+1)*mWidth;

	mHeightData.reserve(cells);
	mDensityData.reserve(cells);
	mVelocityData.reserve(cells);

	mFaces.reserve(faces);
	mCells.reserve(cells);

	for (int y = 0; y < mHeight; y++) {
		for (int x = 0; x < mWidth; x++) {
			Cell* curCell = new Cell(x,y);
			mCells.push_back(curCell);

			Face* horizontal      = CreateFace();
			Face* vertical        = CreateFace();
			curCell->faces[WEST]  = horizontal;
			curCell->faces[NORTH] = vertical;

			// Bind the east face of the cell west of the current cell
			if (x > 0) {
				Cell* westCell         = mCells[GRID_ID(x-1,y)];
				westCell->faces[EAST]  = horizontal;
			}

			// Bind the south face of the cell north of the current cell
			if (y > 0) {
				Cell* northCell         = mCells[GRID_ID(x,y-1)];
				northCell->faces[SOUTH] = vertical;
			}

			// Bind a new face to the southern face of the border cell
			if (y == mHeight-1) {
				curCell->faces[SOUTH] = CreateFace();
			}

			// Bind a new face to the eastern face of the border cell
			if (x == mWidth-1) {
				curCell->faces[EAST]  = CreateFace();
			}

			// Set the height, assuming the world is static wrt height
			curCell->height = ELEVATION(x,y);
			mHeightData[GRID_ID(x,y)] = curCell->height;
		}
	}

	for (int y = 0; y < mHeight; y++) {
		for (int x = 0; x < mWidth; x++) {
			Cell *cell = mCells[GRID_ID(x,y)];
			cell->ResetDynamicVars();

			// Compute delta-heights
			if (y > 0)
				cell->faces[NORTH]->gradHeight = 
					vec3f(-mSquareSize, 0.0f, cell->height - mCells[GRID_ID(x, y-1)]->height);

			if (y < mHeight-1)
				cell->faces[SOUTH]->gradHeight = 
					vec3f( mSquareSize, 0.0f, cell->height - mCells[GRID_ID(x, y+1)]->height);

			if (x > 0)
				cell->faces[WEST]->gradHeight = 
					vec3f(cell->height - mCells[GRID_ID(x-1, y)]->height, 0.0f, -mSquareSize);

			if (x < mWidth-1)
				cell->faces[EAST]->gradHeight = 
					vec3f(cell->height - mCells[GRID_ID(x+1, y)]->height, 0.0f, mSquareSize);
		}
	}
}

void Grid::AddDensityAndVelocity(const vec3f& inPos, const vec3f& inVel) {
	vec3f posf = vec3f(inPos.x/mSquareSize, 0.0f, inPos.z/mSquareSize);
	vec3i posi = Real2Grid(inPos);

	int i = (posf.x > posi.x+0.5f) ? posi.x+1 : posi.x;
	int j = (posf.z > posi.z+0.5f) ? posi.z+1 : posi.z;

	PFFG_ASSERT(i > 0 && j > 0 && i < mWidth && j < mHeight);

	Cell *A = mCells[GRID_ID(i-1, j-1)]; mTouchedCells[GRID_ID(i-1, j-1)] = A; 
	Cell *B = mCells[GRID_ID(i  , j-1)]; mTouchedCells[GRID_ID(i  , j-1)] = B;
	Cell *C = mCells[GRID_ID(i  , j  )]; mTouchedCells[GRID_ID(i  , j  )] = C;
	Cell *D = mCells[GRID_ID(i-1, j  )]; mTouchedCells[GRID_ID(i-1, j  )] = D;

	// Add velocity
	C->avgVelocity += inVel;

	// Compute delta-X and delta-Y
	float dX = posf.x - A->x + 0.5f;
	float dY = posf.z - A->y + 0.5f;

	// Splat density
	A->density += pow(std::min<float>(1.0f - dX, 1.0f - dY), mLambda);
	B->density += pow(std::min<float>(       dX, 1.0f - dY), mLambda);
	C->density += pow(std::min<float>(       dX,        dY), mLambda);
	D->density += pow(std::min<float>(1.0f - dX,        dY), mLambda);
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
		
		const vec3f realPos = Grid2Real(cell);
		for (int dir = 0; dir < DIRECTIONS; dir++) {
			// Compute the speed field
			vec3i dGridPos = Real2Grid(realPos + dirVectors[dir]*maxRadius);
			Cell* dCell = mCells[GRID_ID(dGridPos.x, dGridPos.z)];

			// FIXME: Engine slope should be in the same format as CC slope
			float topologicalSpeed = 
				maxSpeed + ((cell->faces[dir]->gradHeight.dot2D(dirVectors[dir]) - minSlope) / 
				(maxSlope - minSlope)) * 
				(maxSpeed - minSpeed);

			float flowSpeed = 
				dCell->avgVelocity.dot2D(dirVectors[dir]);
			
			float speed = 
				((dCell->density - mMinDensity) / (maxDensity - mMinDensity)) * 
				(topologicalSpeed - flowSpeed) + 
				topologicalSpeed;

			cell->speed[dir] = speed;

			// Compute the unit cost
			cell->cost[dir] = (speedWeight*speed + discomfortWeight*cell->discomfort) / speed;
		}
	}
}

void Grid::UpdateGroupPotentialField(const std::vector<Cell*>& inGoalCells) {
}

void Grid::UpdateSimObjectLocation(const int inSimObjectId) {
}

void Grid::Reset() {
	std::map<unsigned int, Cell*>::iterator i;
	for (i = mTouchedCells.begin(); i != mTouchedCells.end(); i++)
		i->second->ResetDynamicVars();
	
	mTouchedCells.clear();
}

Face* Grid::CreateFace() {
	Face* f = new Face();
	mFaces.push_back(f);
	return f;
}

vec3i Grid::Real2Grid(const vec3f& inVec) {
	return vec3i(int(inVec.x/mSquareSize), 0, int(inVec.z/mSquareSize));
}

vec3f Grid::Grid2Real(const Cell* inCell) {
	return vec3f(inCell->x*mSquareSize, ELEVATION(inCell->x, inCell->y), inCell->y*mSquareSize);
}

Grid::~Grid() {
	for (size_t i = 0; i < mCells.size(); i++)
		delete mCells[i];

	for (size_t i = 0; i < mFaces.size(); i++)
		delete mFaces[i];
}




void Cell::ResetFull() {
	ResetDynamicVars();
	height = 0.0f;
	for (int dir = 0; dir < DIRECTIONS; dir++)
		faces[dir]->gradHeight = NVECf;
}

void Cell::ResetDynamicVars() {
	ResetGroupVars();
	avgVelocity = NVECf;
	density     = 0.0f;
	discomfort = 0.0f;
}

void Cell::ResetGroupVars() {
	potential  = std::numeric_limits<float>::max();
	for (int dir = 0; dir < DIRECTIONS; dir++) {
		speed[dir] = 0.0f;
		cost[dir]  = 0.0f;
		faces[dir]->gradPotential = NVECf;
		faces[dir]->velocity = NVECf;
	}
}
