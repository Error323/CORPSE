#include <SDL/SDL.h>
#include <GL/gl.h>
#include <FTGL/ftgl.h>

#include "./UI.hpp"
#include "./HUDWidget.hpp"
#include "../Math/vec3.hpp"
#include "../Path/IPathModule.hpp"
#include "../Renderer/CameraController.hpp"
#include "../Renderer/Camera.hpp"
#include "../Renderer/RenderThread.hpp"
#include "../Sim/SimThread.hpp"
#include "../Sim/SimObjectHandler.hpp"
#include "../System/EngineAux.hpp"

void ui::HUDWidget::Update(const vec3i&, const vec3i& size) {
	const IPathModule* m = sThread->GetPathModule();
	const Camera* c = rThread->GetCamCon()->GetCurrCam();
	const char* s1 = (c->projMode == Camera::CAM_PROJ_MODE_PERSP) ? "Persp.": "Ortho.";
	const char* s2 = (c->Active()? "active": "inactive");

	static unsigned int tick = SDL_GetTicks();
	static unsigned int sFrame = sThread->GetFrame(), sFPS = 0;
	static unsigned int rFrame = rThread->GetFrame(), rFPS = 0;

	static char sFrameStrBuf[64]          = {'\0'};
	static char rFrameStrBuf[64]          = {'\0'};
	static char camPosStrBuf[128]         = {'\0'};
	static char camDirStrBuf[128]         = {'\0'};
	static char camModeStrBuf[128]        = {'\0'};
	static char mouseLookStrBuf[64]       = {'\0'};
	static char numGroupsStrBuf[64]       = {'\0'};
	static char scalarOverlayStrBuf[128]  = {'\0'};
	static char vectorOverlayStrBuf[128]  = {'\0'};

	static IPathModule::DataTypeInfo scalarOverlayInfo = DATATYPEINFO_CACHED;
	static IPathModule::DataTypeInfo vectorOverlayInfo = DATATYPEINFO_CACHED;

	if ((SDL_GetTicks() - tick) >= 1000) {
		tick   = SDL_GetTicks();
		sFPS   = sThread->GetFrame() - sFrame;
		rFPS   = rThread->GetFrame() - rFrame;
		sFrame = sThread->GetFrame();
		rFrame = rThread->GetFrame();
	}

	// TODO: print "nil" if visualisations (or individual overlays) are disabled
	m->GetScalarDataTypeInfo(&scalarOverlayInfo);
	m->GetVectorDataTypeInfo(&vectorOverlayInfo);

	snprintf(sFrameStrBuf, 64, "s-frame: %u (rate: %u f/s)", sThread->GetFrame(), sFPS);
	snprintf(rFrameStrBuf, 64, "r-frame: %u (rate: %u f/s)", rThread->GetFrame(), rFPS);
	snprintf(camPosStrBuf, 128, "cam-pos: <%.2f, %.2f, %.2f>", c->pos.x, c->pos.y, c->pos.z);
	snprintf(camDirStrBuf, 128, "cam-dir: <%.2f, %.2f, %.2f>", c->zdir.x, c->zdir.y, c->zdir.z);
	snprintf(mouseLookStrBuf, 64, "mouse-look: %s", (AUX->GetMouseLook()? "enabled": "disabled"));
	snprintf(numGroupsStrBuf, 64, "units: %u, groups: %u", simObjectHandler->GetNumSimObjects(), m->GetNumGroupIDs());
	snprintf(scalarOverlayStrBuf, 128, "scalar overlay: %s (group: %u)", scalarOverlayInfo.name, scalarOverlayInfo.group);
	snprintf(vectorOverlayStrBuf, 128, "vector overlay: %s (group: %u)", vectorOverlayInfo.name, vectorOverlayInfo.group);

	switch (c->moveMode) {
		case Camera::CAM_MOVE_MODE_FPS: {
			snprintf(camModeStrBuf, 128, "cam-mode: FPS (%s), %s", s1, s2);
		} break;
		case Camera::CAM_MOVE_MODE_ORBIT: {
			snprintf(camModeStrBuf, 128, "cam-mode: Orbit (%s), %s", s1, s2);
		} break;
		case Camera::CAM_MOVE_MODE_OVERHEAD: {
			snprintf(camModeStrBuf, 128, "cam-mode: Overhead (%s), %s", s1, s2);
		} break;
	}

	const float xmin = -size.x        + 10.0f;
	const float ymax =  size.y * 0.9f - 10.0f;
	const float yoff = float(gUI->GetFont()->FaceSize());

	glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		// text needs to be drawn in absolute coordinates
		glOrtho(-size.x, size.x, -size.y, size.y, -100000.0, 100000.0);
	glMatrixMode(GL_MODELVIEW);
		glPushAttrib(GL_ENABLE_BIT);
		glDisable(GL_DEPTH_TEST);
		glPushMatrix();
			glLoadIdentity();
			gUI->GetFont()->FaceSize(yoff * 0.5f);
			glTranslatef(xmin,  ymax,        0.0f); gUI->GetFont()->Render(sFrameStrBuf);
			glTranslatef(0.0f, -yoff * 0.5f, 0.0f); gUI->GetFont()->Render(rFrameStrBuf);
			glTranslatef(0.0f, -yoff * 0.5f, 0.0f);
			glTranslatef(0.0f, -yoff * 0.5f, 0.0f); gUI->GetFont()->Render(camPosStrBuf);
			glTranslatef(0.0f, -yoff * 0.5f, 0.0f); gUI->GetFont()->Render(camDirStrBuf);
			glTranslatef(0.0f, -yoff * 0.5f, 0.0f); gUI->GetFont()->Render(camModeStrBuf);
			glTranslatef(0.0f, -yoff * 0.5f, 0.0f);
			glTranslatef(0.0f, -yoff * 0.5f, 0.0f); gUI->GetFont()->Render(mouseLookStrBuf);
			glTranslatef(0.0f, -yoff * 0.5f, 0.0f);
			glTranslatef(0.0f, -yoff * 0.5f, 0.0f); gUI->GetFont()->Render(numGroupsStrBuf);
			glTranslatef(0.0f, -yoff * 0.5f, 0.0f);
			glTranslatef(0.0f, -yoff * 0.5f, 0.0f); gUI->GetFont()->Render(scalarOverlayStrBuf);
			glTranslatef(0.0f, -yoff * 0.5f, 0.0f); gUI->GetFont()->Render(vectorOverlayStrBuf);
			gUI->GetFont()->FaceSize(yoff);
		glPopMatrix();
		glPopAttrib();
	glMatrixMode(GL_PROJECTION);
		glPopMatrix();
}
