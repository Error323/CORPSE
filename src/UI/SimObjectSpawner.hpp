#ifndef PFFG_SIMOBJECT_SPAWNER
#define PFFG_SIMOBJECT_SPAWNER

#include "../Math/vec3fwd.hpp"
#include "../Math/vec3.hpp"

namespace ui {
	struct SimObjectSpawner {
	public:
		SimObjectSpawner(): cursorObjID(-1) {
		}

		void MousePressed(int, int, int) {}
		void MouseReleased(int, int, int);
		void MouseMoved(int, int, int, int);

		void Update();

	private:
		vec3f cursorPos;
		vec3f cursorDir;

		unsigned int cursorObjID;
	};
}

#endif
