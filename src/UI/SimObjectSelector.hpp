#ifndef PFFG_SIMOBJECT_SELECTOR_HDR
#define PFFG_SIMOBJECT_SELECTOR_HDR

#include <list>

#include "../Math/vec3fwd.hpp"
#include "../Math/vec3.hpp"

struct SimObjectSelector {
public:
	SimObjectSelector(): haveSelection(false), activeSelection(false) {
	}

	void ClearSelection();
	void StartSelection(int, int);
	void UpdateSelection(int, int);
	void FinishSelection(int, int);
	void DrawSelection();

private:
	bool haveSelection;
	bool activeSelection;

	vec3f selectionStartPos2D;
	vec3f selectionFinishPos2D;

	std::list<unsigned int> selectedObjectIDs;
};

#endif
