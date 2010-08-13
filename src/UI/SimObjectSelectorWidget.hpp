#ifndef PFFG_SIMOBJECT_SELECTOR_HDR
#define PFFG_SIMOBJECT_SELECTOR_HDR

#include <set>

#include "../Math/vec3fwd.hpp"
#include "../Math/vec3.hpp"

#include "./UIWidget.hpp"

namespace ui {
	struct SimObjectSelectorWidget: public IUIWidget {
	public:
		SimObjectSelectorWidget(): haveSelection(false), activeSelection(false), shiftPressed(false) {
		}

		void KeyPressed(int);
		void KeyReleased(int);
		void MouseMoved(int, int, int, int);
		void MousePressed(int, int, int);
		void MouseReleased(int, int, int);

		void Update(const vec3i&, const vec3i&);

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

		// must be a set (objects can occupy multiple cells)
		std::set<unsigned int> selectedObjectIDs;

		bool shiftPressed;
	};
}

#endif
