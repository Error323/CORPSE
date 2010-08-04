#include "./Grid.hpp"

#include "../../System/Debugger.hpp"

void Grid::Init(const unsigned int width, const unsigned int height, const unsigned int squareSize) {
	mWidth = width;
	mHeight = height;
	mSquareSize = squareSize;

	unsigned int cells = width*height;
	unsigned int faces = (width+1)*height + (height+1)*width;

	mFaces.reserve(faces);
	mCells.reserve(cells);

	for (unsigned int y = 0; y < height; y++) {
		for (unsigned int x = 0; x < width; x++) {
			Cell *curCell = new Cell();
			mCells.push_back(curCell);

			Face *horizontal      = CreateFace();
			Face *vertical        = CreateFace();
			curCell->faces[WEST]  = horizontal;
			curCell->faces[NORTH] = vertical;

			// Bind the east face of the cell west of the current cell
			if (x > 0) {
				Cell *westCell         = mCells[y*width+(x-1)];
				westCell->faces[EAST]  = horizontal;
			}

			// Bind the south face of the cell north of the current cell
			if (y > 0) {
				Cell *northCell         = mCells[(y-1)*width+x];
				northCell->faces[SOUTH] = vertical;
			}

			// Bind a new face to the southern face of the border cell
			if (y == height-1) {
				curCell->faces[SOUTH] = CreateFace();
			}

			// Bind a new face to the eastern face of the border cell
			if (x == width-1) {
				curCell->faces[EAST]  = CreateFace();
			}
		}
	}
}

Face* Grid::CreateFace() {
	Face* f = new Face();
	mFaces.push_back(f);
	return f;
}

Grid::~Grid() {
	for (size_t i = 0; i < mCells.size(); i++)
		delete mCells[i];

	for (size_t i = 0; i < mFaces.size(); i++)
		delete mFaces[i];
}
