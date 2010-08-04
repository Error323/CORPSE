#include "./Grid.hpp"

#include <math.h>

#include "../../System/Debugger.hpp"

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

	mHeightData = new float[mWidth*mHeight];

	unsigned int cells = mWidth*mHeight;
	unsigned int faces = (mWidth+1)*mHeight + (mHeight+1)*mWidth;

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
				cell->dHeight[NORTH] = cell->height - mCells[GRID_ID(x, y-1)]->height;

			if (y < mHeight-1)
				cell->dHeight[SOUTH] = cell->height - mCells[GRID_ID(x, y+1)]->height;

			if (x > 0)
				cell->dHeight[WEST]  = cell->height - mCells[GRID_ID(x-1, y)]->height;

			if (x < mWidth-1)
				cell->dHeight[EAST]  = cell->height - mCells[GRID_ID(x+1, y)]->height;
		}
	}
}

void Grid::AddDensityAndVelocity(const vec3f& inPos, const vec3f& inVel) {
	vec3f posf = vec3f(inPos.x/mSquareSize, 0.0f, inPos.z/mSquareSize);
	vec3i posi = Real2Grid(inPos);

	int i = (posf.x > posi.x+0.5f) ? posi.x+1 : posi.x;
	int j = (posf.z > posi.z+0.5f) ? posi.z+1 : posi.z;

	PFFG_ASSERT(i > 0 && j > 0);

	Cell *A = mCells[GRID_ID(i-1, j-1)];
	Cell *B = mCells[GRID_ID(i  , j-1)];
	Cell *C = mCells[GRID_ID(i  , j  )];
	Cell *D = mCells[GRID_ID(i-1, j  )];

	// Add velocity
	C->avgVelocity += inVel;

	// Compute delta-X and delta-Y
	float dX = posf.x - A->x + 0.5f;
	float dY = posf.z - A->y + 0.5f;

	// Splat density
	static const float lambda = 2.0f;
	A->potential += pow(std::min<float>(1.0f - dX, 1.0f - dY), lambda);
	B->potential += pow(std::min<float>(       dX, 1.0f - dY), lambda);
	C->potential += pow(std::min<float>(       dX,        dY), lambda);
	D->potential += pow(std::min<float>(1.0f - dX,        dY), lambda);
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

	delete[] mHeightData;
}
