#include <cstdlib>

#include <SDL/SDL_mouse.h>
#include <GL/gl.h>

#include "./SimObjectSpawnerWidget.hpp"
#include "../Map/Ground.hpp"
#include "../Renderer/RenderThread.hpp"
#include "../Renderer/CameraController.hpp"
#include "../Renderer/Camera.hpp"
#include "../Renderer/Models/ModelReaderBase.hpp"
#include "../Sim/SimCommands.hpp"
#include "../Sim/SimObjectDefHandler.hpp"
#include "../Sim/SimObjectHandler.hpp"
#include "../Sim/SimObjectGrid.hpp"
#include "../Sim/SimObject.hpp"
#include "../System/Client.hpp"
#include "../System/NetMessages.hpp"

void ui::SimObjectSpawnerWidget::MouseReleased(int button, int, int) {
	if (button != SDL_BUTTON_MIDDLE) {
		return;
	}

	const Camera* camera = rThread->GetCamCon()->GetCurrCam();

	if (!camera->Active()) {
		if (simObjectHandler->IsValidSimObjectID(cursorObjID)) {
			NetMessage m(CLIENT_MSG_SIMCOMMAND, client->GetClientID(), (2 * sizeof(unsigned int)));

			// destroy an object
			m << COMMAND_DESTROY_SIMOBJECT;
			m << cursorObjID;

			client->SendNetMessage(m);
		} else {
			NetMessage m(CLIENT_MSG_SIMCOMMAND, client->GetClientID(), (2 * sizeof(unsigned int)) + (6 * sizeof(float)));

			// create an object
			m << COMMAND_CREATE_SIMOBJECT;
			m << static_cast<unsigned int>(random() % simObjectDefHandler->GetNumDefs());
			m << cursorPos.x;
			m << cursorPos.y;
			m << cursorPos.z;
			m << -cursorDir.x;
			m << 0.0f;
			m << -cursorDir.z;

			client->SendNetMessage(m);
		}
	}
}

void ui::SimObjectSpawnerWidget::MouseMoved(int x, int y, int, int) {
	const Camera* camera = rThread->GetCamCon()->GetCurrCam();
	const vec3f& dir = camera->GetPixelDir(x, y);
	const float dst = ground->LineGroundCol(camera->pos, camera->pos + dir * camera->zFarDistance);
	const vec3f pos = camera->pos + dir * dst;
	const SimObject* obj = NULL;

	cursorPos = pos;
	cursorDir = dir;

	if (dst < 0.0f) {
		return;
	}

	obj = simObjectHandler->GetClosestSimObject(pos, 64.0f);

	if (obj != NULL) {
		cursorObjID = obj->GetID();
	} else {
		cursorObjID = -1;
	}
}

void ui::SimObjectSpawnerWidget::Update(const vec3i&, const vec3i&) {
	Camera* camera = rThread->GetCamCon()->GetCurrCam();

	if (!camera->Active()) {
		const SimObject* cursorObj = simObjectHandler->GetClosestSimObject(cursorPos, 64.0f);

		if (cursorObj != NULL && cursorObj->GetID() != cursorObjID) {
			cursorObjID = cursorObj->GetID();
		} else {
			if (!simObjectHandler->IsValidSimObjectID(cursorObjID)) {
				return;
			}
		}

		const SimObject* obj = simObjectHandler->GetSimObject(cursorObjID);
		const mat44f& objMat = obj->GetMat();
		const ModelBase* objMdl = obj->GetModel()->GetModelBase();
		const vec3f objSize = objMdl->maxs - objMdl->mins;

		if ((obj->GetPos() - cursorPos).sqLen3D() > (64.0f * 64.0f)) {
			return;
		}

		glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
		glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

		camera->ApplyViewProjTransform();

		glPushMatrix();
			glMultMatrixf(objMat.m);
			glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);
				glDisable(GL_DEPTH_TEST);
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_COLOR, GL_DST_ALPHA);
				glColor4f(0.0f, 1.0f, 0.0f, 0.25f);
				glBegin(GL_QUADS);
					glVertex3f(-objSize.x * 0.5f, 0.0f, -objSize.z * 0.5f);
					glVertex3f( objSize.x * 0.5f, 0.0f, -objSize.z * 0.5f);
					glVertex3f( objSize.x * 0.5f, 0.0f,  objSize.z * 0.5f);
					glVertex3f(-objSize.x * 0.5f, 0.0f,  objSize.z * 0.5f);
				glEnd();
			glPopAttrib();
		glPopMatrix();

		glMatrixMode(GL_PROJECTION); glPopMatrix();
		glMatrixMode(GL_MODELVIEW); glPopMatrix();
	}
}
