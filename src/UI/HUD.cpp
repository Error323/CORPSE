#include <SDL/SDL.h>
#include <GL/gl.h>
#include <FTGL/ftgl.h>

#include "./UI.hpp"
#include "./HUD.hpp"
#include "../Math/vec3.hpp"
#include "../Renderer/CameraController.hpp"
#include "../Renderer/Camera.hpp"
#include "../Renderer/RenderThread.hpp"
#include "../Sim/SimThread.hpp"
#include "../Sim/SimObjectHandler.hpp"
#include "../System/EngineAux.hpp"

void ui::HUD::Update(const vec3i& pos, const vec3i& size) {
	const Camera* c = renderThread->GetCamCon()->GetCurrCam();
	const char* s1 = (c->projMode == Camera::CAM_PROJ_MODE_PERSP) ? "Persp.": "Ortho.";
	const char* s2 = (c->Active()? "active": "inactive");

	static unsigned int tick = SDL_GetTicks();
	static unsigned int sFrame = simThread->GetFrame(), sFPS = 0;
	static unsigned int rFrame = renderThread->GetFrame(), rFPS = 0;

	static char sFrameBuf[64] = {'\0'};
	static char rFrameBuf[64] = {'\0'};
	static char camPosBuf[128] = {'\0'};
	static char camDirBuf[128] = {'\0'};
	static char camModeBuf[128] = {'\0'};
	static char mouseLookBuf[64] = {'\0'};

	if ((SDL_GetTicks() - tick) >= 1000) {
		tick   = SDL_GetTicks();
		sFPS   = simThread->GetFrame() - sFrame;
		rFPS   = renderThread->GetFrame() - rFrame;
		sFrame = simThread->GetFrame();
		rFrame = renderThread->GetFrame();
	}

	snprintf(sFrameBuf, 64, "s-frame: %u (rate: %u f/s)", simThread->GetFrame(), sFPS);
	snprintf(rFrameBuf, 64, "r-frame: %u (rate: %u f/s)", renderThread->GetFrame(), rFPS);
	snprintf(camPosBuf, 128, "cam-pos: <%.2f, %.2f, %.2f>", c->pos.x, c->pos.y, c->pos.z);
	snprintf(camDirBuf, 128, "cam-dir: <%.2f, %.2f, %.2f>", c->zdir.x, c->zdir.y, c->zdir.z);
	snprintf(mouseLookBuf, 64, "mouse-look: %s", (AUX->GetMouseLook()? "enabled": "disabled"));

	switch (c->moveMode) {
		case Camera::CAM_MOVE_MODE_FPS: {
			snprintf(camModeBuf, 128, "cam-mode: FPS (%s), %s", s1, s2);
		} break;
		case Camera::CAM_MOVE_MODE_ORBIT: {
			snprintf(camModeBuf, 128, "cam-mode: Orbit (%s), %s", s1, s2);
		} break;
		case Camera::CAM_MOVE_MODE_OVERHEAD: {
			snprintf(camModeBuf, 128, "cam-mode: Overhead (%s), %s", s1, s2);
		} break;
	}

	const float xmin = -size.x        + 10.0f;
	const float ymax =  size.y * 0.9f - 10.0f;
	const float yoff = float(gUI->GetFont()->FaceSize());

	glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(-size.x, size.x, -size.y, size.y, -100000.0, 100000.0);
	glMatrixMode(GL_MODELVIEW);
		glPushAttrib(GL_ENABLE_BIT);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_TEXTURE_2D);
		glPushMatrix();
			glLoadIdentity();
			gUI->GetFont()->FaceSize(yoff * 0.5f);
			glTranslatef(xmin,  ymax,        0.0f); gUI->GetFont()->Render(sFrameBuf);
			glTranslatef(0.0f, -yoff * 0.5f, 0.0f); gUI->GetFont()->Render(rFrameBuf);
			glTranslatef(0.0f, -yoff * 0.5f, 0.0f);
			glTranslatef(0.0f, -yoff * 0.5f, 0.0f); gUI->GetFont()->Render(camPosBuf);
			glTranslatef(0.0f, -yoff * 0.5f, 0.0f); gUI->GetFont()->Render(camDirBuf);
			glTranslatef(0.0f, -yoff * 0.5f, 0.0f); gUI->GetFont()->Render(camModeBuf);
			glTranslatef(0.0f, -yoff * 0.5f, 0.0f);
			glTranslatef(0.0f, -yoff * 0.5f, 0.0f); gUI->GetFont()->Render(mouseLookBuf);
			gUI->GetFont()->FaceSize(yoff);
		glPopMatrix();
		glPopAttrib();
	glMatrixMode(GL_PROJECTION);
		glPopMatrix();
}
