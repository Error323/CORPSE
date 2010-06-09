#include <cstdlib>

#include "./SimObjectSpawner.hpp"
#include "../Map/Ground.hpp"
#include "../Renderer/RenderThread.hpp"
#include "../Renderer/CameraController.hpp"
#include "../Renderer/Camera.hpp"
#include "../Sim/SimCommands.hpp"
#include "../Sim/SimObjectDefHandler.hpp"
#include "../Sim/SimObjectHandler.hpp"
#include "../Sim/SimObjectGrid.hpp"
#include "../Sim/SimObject.hpp"
#include "../System/Client.hpp"
#include "../System/NetMessages.hpp"

void SimObjectSpawner::SpawnObject(int x, int y) {
	const Camera* camera = renderThread->GetCamCon()->GetCurrCam();

	if (!camera->Active()) {
		SimObjectGrid<const SimObject*>* grid = simObjectHandler->GetSimObjectGrid();

		const vec3f& dir = camera->GetPixelDir(x, y);
		const float dst = ground->LineGroundCol(camera->pos, camera->pos + dir * camera->zFarDistance);
		const vec3f pos = camera->pos + dir * dst;

		if (dst > 0.0f) {
			const vec3i& idx = grid->GetCellIdx(pos, true);

			const SimObjectGrid<const SimObject*>::GridCell& cell = grid->GetCell(idx);
			const std::list<const SimObject*> objects = cell.GetObjects();
			const SimObject* closestObject = NULL;

			for (std::list<const SimObject*>::const_iterator it = objects.begin(); it != objects.end(); ++it) {
				const vec3f& objPos = (*it)->GetPos();

				if ((closestObject == NULL) || ((objPos - pos).sqLen3D() < (closestObject->GetPos() - pos).sqLen3D())) {
					closestObject = *it;
				}
			}

			if (closestObject != NULL) {
				NetMessage m(CLIENT_MSG_SIMCOMMAND, (2 * sizeof(unsigned int)));

				// destroy an object
				m << COMMAND_DESTROY_SIMOBJECT;
				m << closestObject->GetID();

				client->SendNetMessage(m);
			} else {
				NetMessage m(CLIENT_MSG_SIMCOMMAND, (2 * sizeof(unsigned int)) + (6 * sizeof(float)));

				// create an object
				m << COMMAND_CREATE_SIMOBJECT;
				m << (random() % simObjectDefHandler->GetNumDefs());
				m << pos.x;
				m << pos.y;
				m << pos.z;
				m << -dir.x;
				m <<    0.0f;
				m << -dir.z;

				// client->SendNetMessage(m);
			}
		}
	}
}
