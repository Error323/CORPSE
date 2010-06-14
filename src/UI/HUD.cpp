#include <GL/gl.h>
#include <FTGL/ftgl.h>

#include "./UI.hpp"
#include "./HUD.hpp"
#include "../Math/vec3.hpp"
#include "../Sim/SimThread.hpp"
#include "../Renderer/RenderThread.hpp"

void ui::HUD::Update(const vec3i& pos, const vec3i& size) {
	static char sFrameCounter[64] = {'\0'};
	static char rFrameCounter[64] = {'\0'};

	snprintf(sFrameCounter, 64, "s-frame: %u (avg: %.2f)", simThread->GetFrame(), 0.0f);
	snprintf(rFrameCounter, 64, "r-frame: %u (avg: %.2f)", renderThread->GetFrame(), 0.0f);

	const float xmin = -size.x        + 10.0f;
	const float ymin =  size.y * 0.9f - 10.0f;

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
			glTranslatef(xmin,  ymin, 0.0f); gUI->GetFont()->Render(sFrameCounter);
			glTranslatef(0.0f, -yoff, 0.0f); gUI->GetFont()->Render(rFrameCounter);
		glPopMatrix();
		glPopAttrib();
	glMatrixMode(GL_PROJECTION);
		glPopMatrix();
}
