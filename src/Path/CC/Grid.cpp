#include "./Grid.hpp"

#include "../../System/Debugger.hpp"

#define GRID_ID(x,y) (((y)*(mWidth))+(x))

void Grid::Init(const int res, ICallOutHandler* coh) {
	PFFG_ASSERT(res >= 1);

	mCoh        = coh;
	mWidth      = mCoh->GetHeightMapSizeX() / res;
	mHeight     = mCoh->GetHeightMapSizeZ() / res;
	mSquareSize = mCoh->GetSquareSize()     * res;
	printf("[Grid::Init] GridRes: %dx%d %d\n", mWidth, mHeight, mSquareSize);

	unsigned int cells = mWidth*mHeight;
	unsigned int faces = (mWidth+1)*mHeight + (mHeight+1)*mWidth;

	mFaces.reserve(faces);
	mCells.reserve(cells);

	for (int y = 0; y < mHeight; y++) {
		for (int x = 0; x < mWidth; x++) {
			Cell *curCell = new Cell(x,y);
			mCells.push_back(curCell);

			Face *horizontal      = CreateFace();
			Face *vertical        = CreateFace();
			curCell->faces[WEST]  = horizontal;
			curCell->faces[NORTH] = vertical;

			// Bind the east face of the cell west of the current cell
			if (x > 0) {
				Cell *westCell         = mCells[GRID_ID(x-1,y)];
				westCell->faces[EAST]  = horizontal;
			}

			// Bind the south face of the cell north of the current cell
			if (y > 0) {
				Cell *northCell         = mCells[GRID_ID(x,y-1)];
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
			curCell->height = mCoh->GetCenterHeightMap()[(res*y)*(res*mWidth)+(res*x)];
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

void Grid::AddDensityAndSpeed(const vec3f& inPos, const vec3f& inVel) {
	vec3f pos = Real2Grid(inPos);
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

vec3f Grid::Real2Grid(const vec3f& v) {
	return vec3f(v.x/mSquareSize, 0.0f, v.z/mSquareSize);
}

Grid::~Grid() {
	for (size_t i = 0; i < mCells.size(); i++)
		delete mCells[i];

	for (size_t i = 0; i < mFaces.size(); i++)
		delete mFaces[i];
}
