#include <cassert>

#include <GL/gl.h>

#include "./SimObjectSelector.hpp"
#include "../Map/Ground.hpp"
#include "../Renderer/RenderThread.hpp"
#include "../Renderer/CameraController.hpp"
#include "../Renderer/Camera.hpp"
#include "../Sim/SimObjectHandler.hpp"
#include "../System/EngineAux.hpp"

void SimObjectSelector::ClearSelection() {
	selectedObjectIDs.clear();

	selectionStartPos2D.x = selectionStartPos2D.y = -1.0f;
	selectionFinishPos2D.x = selectionFinishPos2D.y = -1.0f;

	haveSelection = false;
	activeSelection = false;
}

void SimObjectSelector::StartSelection(int x, int y) {
	const Camera* camera = renderThread->GetCamCon()->GetCurrCam();

	if (!camera->Active()) {
		ClearSelection();

		selectionStartPos2D.x = x; selectionFinishPos2D.x = x;
		selectionStartPos2D.y = y; selectionFinishPos2D.y = y;

		haveSelection = true;
		activeSelection = true;
	}
}

void SimObjectSelector::UpdateSelection(int x, int y) {
	const Camera* camera = renderThread->GetCamCon()->GetCurrCam();

	if (!camera->Active() && haveSelection && activeSelection) {
		selectionFinishPos2D.x = x;
		selectionFinishPos2D.y = y;
	}
}

void SimObjectSelector::FinishSelection(int x, int y) {
	const Camera* camera = renderThread->GetCamCon()->GetCurrCam();

	if (!camera->Active()) {
		assert(haveSelection);
		// when releasing the mouse, we want to preserve the
		// current selection (if any) but not keep it active
		activeSelection = false;

		const int w = int(selectionFinishPos2D.x - selectionStartPos2D.x);
		const int h = int(selectionFinishPos2D.y - selectionStartPos2D.y);

		if ((w >= -2 && w <= 2) || (h >= -2 && h <= 2)) {
			ClearSelection();
		} else {
			const vec3f& tlDir = camera->GetPixelDir(x,     y    );
			const vec3f& trDir = camera->GetPixelDir(x + w, y    );
			const vec3f& brDir = camera->GetPixelDir(x + w, y + h);
			const vec3f& blDir = camera->GetPixelDir(x,     y + h);

			const float tlDirDst = ground->LineGroundCol(camera->pos, camera->pos + tlDir * camera->zFarDistance);
			const float trDirDst = ground->LineGroundCol(camera->pos, camera->pos + trDir * camera->zFarDistance);
			const float brDirDst = ground->LineGroundCol(camera->pos, camera->pos + brDir * camera->zFarDistance);
			const float blDirDst = ground->LineGroundCol(camera->pos, camera->pos + blDir * camera->zFarDistance);

			if (tlDirDst > 0.0f && trDirDst > 0.0f && brDirDst > 0.0f && blDirDst > 0.0f) {
				// all four rays must intersect the ground
				const vec3f tlPos = camera->pos + tlDir * tlDirDst;
				const vec3f trPos = camera->pos + trDir * trDirDst;
				const vec3f brPos = camera->pos + brDir * brDirDst;
				const vec3f blPos = camera->pos + blDir * blDirDst;

				// AA bounding-box for the selection-rectangle
				vec3f mins;
					mins.x = std::min(tlPos.x, std::min(trPos.x, std::min(brPos.x, blPos.x)));
					mins.z = std::min(tlPos.z, std::min(trPos.z, std::min(brPos.z, blPos.z)));
				vec3f maxs;
					maxs.x = std::max(tlPos.x, std::max(trPos.x, std::max(brPos.x, blPos.x)));
					maxs.z = std::max(tlPos.z, std::max(trPos.z, std::max(brPos.z, blPos.z)));
			}
		}
	}
}

// draw the selection rectangle while it is being created
// (in relative screen-space, coordinate range is [-1, 1])
void SimObjectSelector::DrawSelection() {
	if (activeSelection) {
		glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
		glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

		const Camera* camera = renderThread->GetCamCon()->GetCurrCam();

		const int w = int(selectionFinishPos2D.x - selectionStartPos2D.x);
		const int h = int(selectionFinishPos2D.y - selectionStartPos2D.y);

		const unsigned int hvpsx = (WIN->GetViewPortSizeX() >> 1);
		const unsigned int hvpsy = (WIN->GetViewPortSizeY() >> 1);

		const float tlxc = selectionStartPos2D.x     - hvpsx, tlyc = (selectionStartPos2D.y     - hvpsy) * -1.0f;
		const float trxc = selectionStartPos2D.x + w - hvpsx, tryc = (selectionStartPos2D.y     - hvpsy) * -1.0f;
		const float brxc = selectionStartPos2D.x + w - hvpsx, bryc = (selectionStartPos2D.y + h - hvpsy) * -1.0f;
		const float blxc = selectionStartPos2D.x     - hvpsx, blyc = (selectionStartPos2D.y + h - hvpsy) * -1.0f;

		glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LINE_BIT);
			glDisable(GL_DEPTH_TEST);
			glColor3f(1.0f, 0.0f, 0.0f);
			glLineWidth(2.0f);
			glBegin(GL_LINE_LOOP);
				glVertex2f(tlxc / hvpsx, tlyc / hvpsy);
				glVertex2f(trxc / hvpsx, tryc / hvpsy);
				glVertex2f(brxc / hvpsx, bryc / hvpsy);
				glVertex2f(blxc / hvpsx, blyc / hvpsy);
			glEnd();
		glPopAttrib();

		glMatrixMode(GL_PROJECTION); glPopMatrix();
		glMatrixMode(GL_MODELVIEW); glPopMatrix();
	} else {
		if (haveSelection) {
			// draw marker squares around selected objects
			for (std::list<unsigned int>::const_iterator it = selectedObjectIDs.begin(); it != selectedObjectIDs.end(); ++it) {
				if (simObjectHandler->IsValidSimObjectID(*it)) {
					const SimObject* o = simObjectHandler->GetSimObject(*it);
				}
			}
		}
	}
}
