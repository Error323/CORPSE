#ifndef PFFG_GRID_HDR
#define PFFG_GRID_HDR

#include <vector>

enum { NORTH=0, EAST=1, SOUTH=2, WEST=3, DIRECTIONS=4 };

class Face;
class Cell;
class Grid {
public:
	Grid(){}
	~Grid();

	void Init(const unsigned int, const unsigned int, const unsigned int);

private:
	int mWidth;
	int mHeight;
	int mSquareSize;

	std::vector<Cell*> mCells;
	std::vector<Face*> mFaces;

	Face* CreateFace();
};

struct Cell {
	int   x, y;
	float discomfort;
	float potential;
	float density;
	float height;
	float avgVelocity[2];
	float speed[DIRECTIONS];
	float cost[DIRECTIONS];
	Face* faces[DIRECTIONS];
};

struct Face {
	float heightGradient;
	float potentialGradient;
	float velocity[2];
};

#endif
