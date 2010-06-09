#include <cassert>

#include <GL/gl.h>

#include "./SimObjectSelector.hpp"
#include "../Map/Ground.hpp"
#include "../Math/Geom.hpp"
#include "../Renderer/RenderThread.hpp"
#include "../Renderer/CameraController.hpp"
#include "../Renderer/Camera.hpp"
#include "../Sim/SimObjectHandler.hpp"
#include "../Sim/SimObjectGrid.hpp"
#include "../Sim/SimObject.hpp"
#include "../System/EngineAux.hpp"

void SimObjectSelector::ClearSelection() {
	selectedObjectIDs.clear();

	selectionStartPos2D.x = selectionFinishPos2D.x = -1.0f;
	selectionStartPos2D.y = selectionFinishPos2D.y = -1.0f;

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

	if (!haveSelection) { return; }
	if (!activeSelection) { return; }

	if (!camera->Active()) {
		selectionFinishPos2D.x = x;
		selectionFinishPos2D.y = y;

		selectionSquareSize2D = selectionFinishPos2D - selectionStartPos2D;

		const int w = selectionSquareSize2D.x;
		const int h = selectionSquareSize2D.y;

		selectionDirs3D[0] = camera->GetPixelDir(x - w, y - h); // top-left SS corner
		selectionDirs3D[1] = camera->GetPixelDir(x    , y - h); // top-right SS corner
		selectionDirs3D[2] = camera->GetPixelDir(x    , y    ); // bottom-right SS corner
		selectionDirs3D[3] = camera->GetPixelDir(x - w, y    ); // bottom-left SS corner

		selectionDists[0] = ground->LineGroundCol(camera->pos, camera->pos + selectionDirs3D[0] * camera->zFarDistance);
		selectionDists[1] = ground->LineGroundCol(camera->pos, camera->pos + selectionDirs3D[1] * camera->zFarDistance);
		selectionDists[2] = ground->LineGroundCol(camera->pos, camera->pos + selectionDirs3D[2] * camera->zFarDistance);
		selectionDists[3] = ground->LineGroundCol(camera->pos, camera->pos + selectionDirs3D[3] * camera->zFarDistance);

		selectionCoors3D[0] = camera->pos + selectionDirs3D[0] * selectionDists[0];
		selectionCoors3D[1] = camera->pos + selectionDirs3D[1] * selectionDists[1];
		selectionCoors3D[2] = camera->pos + selectionDirs3D[2] * selectionDists[2];
		selectionCoors3D[3] = camera->pos + selectionDirs3D[3] * selectionDists[3];

		const vec3f& tlPos = selectionCoors3D[0];
		const vec3f& trPos = selectionCoors3D[1];
		const vec3f& blPos = selectionCoors3D[2];
		const vec3f& brPos = selectionCoors3D[3];

		selectionBounds3D[0] = NVECf;
		selectionBounds3D[1] = NVECf;

		// AA bounding-box for the selection-rectangle corners
		vec3f& mins = selectionBounds3D[0];
			mins.x = std::min(tlPos.x, std::min(trPos.x, std::min(brPos.x, blPos.x)));
			mins.y = std::min(tlPos.y, std::min(trPos.y, std::min(brPos.y, blPos.y)));
			mins.z = std::min(tlPos.z, std::min(trPos.z, std::min(brPos.z, blPos.z)));
		vec3f& maxs = selectionBounds3D[1];
			maxs.x = std::max(tlPos.x, std::max(trPos.x, std::max(brPos.x, blPos.x)));
			maxs.y = std::max(tlPos.y, std::max(trPos.y, std::max(brPos.y, blPos.y)));
			maxs.z = std::max(tlPos.z, std::max(trPos.z, std::max(brPos.z, blPos.z)));
	}
}

void SimObjectSelector::FinishSelection(int x, int y) {
	const Camera* camera = renderThread->GetCamCon()->GetCurrCam();

	if (!camera->Active()) {
		assert(haveSelection);
		// when releasing the mouse, we want to preserve the
		// current selection (if any) but not keep it active
		activeSelection = false;

		const int w = selectionSquareSize2D.x;
		const int h = selectionSquareSize2D.y;

		// ignore spurious selections
		if (selectionSquareSize2D.x >= -2 && selectionSquareSize2D.x <= 2) { ClearSelection(); return; }
		if (selectionSquareSize2D.y >= -2 && selectionSquareSize2D.y <= 2) { ClearSelection(); return; }

		if (selectionDists[0] > 0.0f && selectionDists[1] > 0.0f && selectionDists[2] > 0.0f && selectionDists[3] > 0.0f) {
			// all four rays must have intersected the ground
			SimObjectGrid<const SimObject*>* grid = simObjectHandler->GetSimObjectGrid();

			const vec3i minsIdx = grid->GetCellIdx(selectionBounds3D[0], true);
			const vec3i maxsIdx = grid->GetCellIdx(selectionBounds3D[1], true);

			// visit the grid-cells within the bounding-box
			for (int i = minsIdx.x; i <= maxsIdx.x; i++) {
				for (int j = minsIdx.z; j <= maxsIdx.z; j++) {
					const SimObjectGrid<const SimObject*>::GridCell& cell = grid->GetCell(vec3i(i, 0, j));
					const std::list<const SimObject*> objects = cell.GetObjects();

					for (std::list<const SimObject*>::const_iterator it = objects.begin(); it != objects.end(); ++it) {
						const vec3f& pos = (*it)->GetPos();

						const float edgeDist0 = geom::PointLineDistance(selectionCoors3D[1], selectionCoors3D[0], pos);
						const float edgeDist1 = geom::PointLineDistance(selectionCoors3D[2], selectionCoors3D[1], pos);
						const float edgeDist2 = geom::PointLineDistance(selectionCoors3D[3], selectionCoors3D[2], pos);
						const float edgeDist3 = geom::PointLineDistance(selectionCoors3D[0], selectionCoors3D[3], pos);

						const bool b0 = (edgeDist0 < 0.0f && edgeDist1 < 0.0f && edgeDist2 < 0.0f && edgeDist3 < 0.0f);
						const bool b1 = (edgeDist0 > 0.0f && edgeDist1 > 0.0f && edgeDist2 > 0.0f && edgeDist3 > 0.0f);

						if (b0 || b1) {
							selectedObjectIDs.push_back((*it)->GetID());
						}
					}
				}
			}
		}
	}
}

void SimObjectSelector::DrawSelection() {
	if (activeSelection) {
		// draw the selection rectangle while it is being created
		// (in relative screen-space, coordinate range is [-1, 1])
		glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
		glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

		Camera* camera = renderThread->GetCamCon()->GetCurrCam();

		const int w = selectionSquareSize2D.x;
		const int h = selectionSquareSize2D.y;

		// we need the half-sizes to map screen-space coordinates to [-1, 1]
		const unsigned int hvpsx = (WIN->GetViewPortSizeX() >> 1);
		const unsigned int hvpsy = (WIN->GetViewPortSizeY() >> 1);

		// convert from [0, vps*] to [-hvps*, hvps*]; convert SDL-y to OGL-y
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


		const vec3f& p0 = selectionCoors3D[0];
		const vec3f& p1 = selectionCoors3D[1];
		const vec3f& p2 = selectionCoors3D[2];
		const vec3f& p3 = selectionCoors3D[3];

		// draw the quad between the four WS positions
		camera->ApplyViewProjTransform();

		glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glColor4f(1.0f, 0.0f, 0.0f, 0.25f);
			glBegin(GL_QUADS);
				glVertex3f(p0.x, p0.y, p0.z);
				glVertex3f(p1.x, p1.y, p1.z);
				glVertex3f(p2.x, p2.y, p2.z);
				glVertex3f(p3.x, p3.y, p3.z);
			glEnd();
		glPopAttrib();


		glMatrixMode(GL_PROJECTION); glPopMatrix();
		glMatrixMode(GL_MODELVIEW); glPopMatrix();
	} else {
		if (haveSelection) {
			// draw marker squares around selected objects
			for (std::list<unsigned int>::const_iterator it = selectedObjectIDs.begin(); it != selectedObjectIDs.end(); ++it) {
				if (simObjectHandler->IsValidSimObjectID(*it)) {
					const SimObject* o = simObjectHandler->GetSimObject(*it); // TODO
				}
			}
		}
	}
}
