/*
#ifndef PFFG_FLOWGRID_HDR
#define PFFG_FLOWGRID_HDR

#include <vector>
#include <map>

#include "../../Math/vec3fwd.hpp"
#include "../../Math/vec3.hpp"

class ICallOutHandler;
class FlowGrid {
public:
	struct FlowCell {
		FlowCell(): numObjects(0), tempID(0) {
		}

		// xz represent direction, y represents strength
		vec3f flowVelocity;

		// basic density measure
		unsigned int numObjects;

		// used to prevent double flow
		unsigned int tempID;
	};

	FlowGrid(ICallOutHandler*);
	~FlowGrid() { cells.clear(); }
	void Reset();
	void AddFlow(unsigned int, unsigned int);
	void AvgFlow();

private:
	unsigned int numCellsX;
	unsigned int numCellsZ;
	unsigned int cellScale;
	unsigned int cellSize;

	std::vector<FlowCell> cells;
	std::set<unsigned int> indices;

	ICallOutHandler* mCOH;
};

#endif
*/
