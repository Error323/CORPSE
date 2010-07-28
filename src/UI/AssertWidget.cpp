#include <SDL/SDL.h>
#include <GL/gl.h>
#include <FTGL/ftgl.h>

#include "./UI.hpp"
#include "./AssertWidget.hpp"
#include "../Math/vec3.hpp"
#include "../Renderer/RenderThread.hpp"
#include "../Sim/SimThread.hpp"
#include "../Sim/SimObjectHandler.hpp"
#include "../System/EngineAux.hpp"

ui::AssertWidget* ui::AssertWidget::mInstance = NULL;

ui::AssertWidget* ui::AssertWidget::GetInstance() {
	if (mInstance == NULL)
		mInstance = new AssertWidget();
	return mInstance;
}

void ui::AssertWidget::Update(const vec3i&, const vec3i& size) {
	if (!mIsEnabled)
		return;


	const float yoff = float(gUI->GetFont()->FaceSize());
	static const float xoff = 100.0f;
	glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(-size.x, size.x, -size.y, size.y, -100000.0, 100000.0);
	glMatrixMode(GL_MODELVIEW);
		glPushAttrib(GL_ENABLE_BIT);
		glDisable(GL_DEPTH_TEST);
		glPushMatrix();
			glLoadIdentity();
			glColor3f(1.0f, 0.0f, 0.0f);
			glRectf(-size.x*0.9f,-size.y*0.9f,size.x*0.9f,size.y*0.9f);
		glPopMatrix();
		glPushMatrix();
			glColor3f(1.0f, 1.0f, 1.0f);
			glLoadIdentity();
			gUI->GetFont()->FaceSize(yoff * 0.5f);
			glTranslatef(-size.x*0.6f, size.y*0.8f, 0.0f);

			int tabbedCount = 0;
			for (size_t j = 0; j < mMessage.size(); j++) {
				if (mMessage[j] == '\n') {
					glTranslatef(-tabbedCount*xoff, -yoff * 0.5f, 0.0f);
					tabbedCount = 0;
				}
				else 
				if (mMessage[j] == '\t') {
					tabbedCount++;
					glTranslatef(xoff, 0.0f, 0.0f);
				}
				else {
					size_t k = j;
					while (true) {
						j++;
						if (j >= mMessage.size())
							break;
						if (mMessage[j] == '\n' || mMessage[j] == '\t')
							break;
					}
					j--;
					gUI->GetFont()->Render(mMessage.substr(k, j+1-k).c_str());
				}
			}
			gUI->GetFont()->FaceSize(yoff);
		glPopMatrix();
		glPopAttrib();
	glMatrixMode(GL_PROJECTION);
		glPopMatrix();
}
