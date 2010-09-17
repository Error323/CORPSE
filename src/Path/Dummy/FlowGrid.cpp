/*
#include "./FlowGrid.hpp"

#define EPSILON 0.01f
#define CELLS_IN_RADIUS(r) ((r / (cellSize >> 1)) + 1)

// convert a 3D world-space position to 1D cell-index
#define CELL_INDEX_1D(p) (((p.z / cellSize) * numCellsX) + (p.x / cellSize))
// convert a 2D (x, z) grid-index to 1D cell-index
#define CELL_INDEX_2D(x, z) (((z) * numCellsX) + (x))

FlowGrid::FlowGrid(ICallOutHandler* coh): mCOH(coh) {
	cellScale = 4;                                     // down-scale factor wrt. height-map
	cellSize  = cellScale * mCOH->GetSquareSize();     // size in world-coors
	numCellsX = mCOH->GetHeightMapSizeX() / cellScale;
	numCellsZ = mCOH->GetHeightMapSizeZ() / cellScale;
}

void FlowGrid::Reset() {
	for (std::set<unsigned int>::const_iterator it = indices.begin(); it != indices.end(); ++it) {
		cells[(*it)].flowVelocity = NVECf;
		cells[(*it)].numObjects = 0;
	}

	indices.clear();
}

void FlowGrid::AddFlow(unsigned int objectID, unsigned int numFlowSteps) {
	static unsigned int tempID = -1;

	const vec3f& pos = mCOH->GetSimObjectPosition(objectID);
	const vec3f& dir = mCOH->GetSimObjectDirection(objectID);
	const float  spd = mCOH->GetSimObjectSpeed(objectID);
	const float  rad = mCOH->GetSimObjectModelRadius(objectID);

	// radius is in world-space, span in grid-space
	const unsigned int span = CELLS_IN_RADIUS(rad);

	for (unsigned int n = 0; n <= numFlowSteps; n++) {
			for (int x = -int(span); x <= int(span); x++) {
				for (int z = -int(span); z <= int(span); z++) {
					const vec3f p = pos + (dir * spd * n);

					const int cx = (p.x / cellSize) + x;
					const int cz = (p.z / cellSize) + z;
					const int idx = (cz + z) * numCellsX + (cx + x);

					if (cx < 0 || cx >= int(numCellsX)) { continue; }
					if (cz < 0 || cz >= int(numCellsZ)) { continue; }

					if ((x * x) + (z * z) > (span * span)) {
						continue;
					}

					FlowCell& cell = cells[idx];

					if (cell.tempID == tempID) {
						continue;
					}

					indices.insert(idx);

					cell.flowVelocity.x += (dir.x * spd);
					cell.flowVelocity.z += (dir.z * spd);
					cell.flowVelocity.y += 1.0f; // TODO
					cell.numObjects += 1;
					cell.tempID = tempID;
				}
			}
	}

	tempID += 1;
}

void FlowGrid::AvgFlow() {
	for (std::set<unsigned int>::const_iterator it = indices.begin(); it != indices.end(); ++it) {
		if (cells[(*it)].flowVelocity.sqLen2D() > EPSILON) {
			cells[(*it)].flowVelocity.inorm2D();
		}
	}
}
*/
