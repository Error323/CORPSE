#ifndef PFFG_SIMOBJECT_SELECTOR_HDR
#define PFFG_SIMOBJECT_SELECTOR_HDR

#include <list>

#include "../Math/vec3fwd.hpp"
#include "../Math/vec3.hpp"

namespace ui {
	struct SimObjectSelector {
	public:
		SimObjectSelector(): haveSelection(false), activeSelection(false) {
		}

		void MouseMoved(int, int, int, int);
		void MousePressed(int, int, int);
		void MouseReleased(int, int, int);
		void Update();

	private:
		void ClearSelection();
		void FillSelection();
		void OrderSelection(int, int);

		bool haveSelection;
		bool activeSelection;

		vec3f selectionStartPos2D;
		vec3f selectionFinishPos2D;
		vec3f selectionSquareSize2D;

		float selectionDists[4];
		vec3f selectionDirs3D[4];
		vec3f selectionCoors3D[4];
		vec3f selectionBounds3D[2];

		std::list<unsigned int> selectedObjectIDs;
	};
}

#endif
