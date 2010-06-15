#ifndef PFFG_SIMOBJECT_SPAWNER
#define PFFG_SIMOBJECT_SPAWNER

#include "../Math/vec3fwd.hpp"
#include "../Math/vec3.hpp"

#include "./UIWidget.hpp"

namespace ui {
	struct SimObjectSpawnerWidget: public IUIWidget {
	public:
		SimObjectSpawnerWidget(): cursorObjID(-1) {
		}

		void MousePressed(int, int, int) {}
		void MouseReleased(int, int, int);
		void MouseMoved(int, int, int, int);

		void Update(const vec3i&, const vec3i&);

	private:
		vec3f cursorPos;
		vec3f cursorDir;

		unsigned int cursorObjID;
	};
}

#endif
