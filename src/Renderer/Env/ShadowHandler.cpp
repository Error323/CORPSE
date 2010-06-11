#include <GL/glew.h>
#include <GL/gl.h>

#include "../../System/EngineAux.hpp"
#include "../../System/LuaParser.hpp"
#include "../../System/Logger.hpp"
#include "../../UI/Window.hpp"
#include "../Shaders/ShaderHandler.hpp"
#include "../Camera.hpp"
#include "./ShadowHandler.hpp"

CShadowHandler::CShadowHandler() {
	fboID = 0;
	texID = 0;

	SIZE_MULT_X = 4;
	SIZE_MULT_Y = 4;

	basicShadows = true;
	enabled = false;
}

CShadowHandler* CShadowHandler::GetInstance() {
	static CShadowHandler sh;
	return &sh;
}

void CShadowHandler::GenDepthTextureFBO() {
	const int shadowMapWidth = gWindow->GetViewPort().size.x * SIZE_MULT_X;
	const int shadowMapHeight = gWindow->GetViewPort().size.y * SIZE_MULT_Y;

	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);


	if (basicShadows) {
		// GL_LINEAR does not make sense for a depth-texture
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	if (!basicShadows) {
		// PCF and VSM shadows need shadow2DProj(), which is enabled
		// by setting these parameters (note: when left active with
		// basic SM, self-shadowing occurs everywhere!)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
	}


	// Remove artefact on the edges of the shadowmap
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	// GLfloat borderColor[4] = {0, 0, 0, 0};
	// glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	// no need to force GL_DEPTH_COMPONENT{24|32}, drivers usually give max precision
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowMapWidth, shadowMapHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffersEXT(1, &fboID);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboID);

	// we won't bind a color texture to this FBO
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	// attach the texture to FBO depth attachment point
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, texID, 0);

	if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT) {
		LOG << "[SHGenerateShadowFBO] GL_FRAMEBUFFER_COMPLETE_EXT not satisfied\n";
	} else {
		enabled = true;
	}

	// switch back to framebuffer provided by window-system
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}



void CShadowHandler::GenShadowProjectionMatrix(Camera* cam) {
	if (!enabled)
		return;

	// transform every coordinate x, y, z by
	//    x = x * 0.5 + 0.5 
	//    y = y * 0.5 + 0.5 
	//    z = z * 0.5 + 0.5 
	// to map the unit-cube [-1, 1] to [0, 1]  
	static const float biasMat[16] = {
		0.5f, 0.0f, 0.0f, 0.0f, 
		0.0f, 0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.5f, 0.5f, 0.5f, 1.0f
	};

	const float* viewMat = cam->GetViewMatrix();
	const float* projMat = cam->GetProjMatrix();

	// more elegant than using the texture-matrix stack
	shadowProjectionMatrix = shadowProjectionMatrix.LoadIdentity();
	shadowProjectionMatrix = shadowProjectionMatrix.Mul(biasMat);
	shadowProjectionMatrix = shadowProjectionMatrix.Mul(projMat);
	shadowProjectionMatrix = shadowProjectionMatrix.Mul(viewMat);

	/*
	// store the shadow-projection matrix on the texture-matrix stack
	// (requires accessing gl_TextureMatrix[7] shader-side, but saves
	// a uniform variable)
	glActiveTexture(GL_TEXTURE7);
	glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glLoadMatrixf(biasMat);
		glMultMatrixf(projMat);
		glMultMatrixf(viewMat);
	glMatrixMode(GL_MODELVIEW);
	glActiveTexture(GL_TEXTURE0);
	*/
}



void CShadowHandler::BindDepthTextureFBO() {
	if (!enabled)
		return;

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboID);

	// if we render the shadowmap in higher res.,
	// the viewport must be modified accordingly
	glViewport(
		gWindow->GetViewPort().pos.x,
		gWindow->GetViewPort().pos.y,
		gWindow->GetViewPort().size.x * SIZE_MULT_X,
		gWindow->GetViewPort().size.y * SIZE_MULT_Y
	);

	// clear previous frame values
	glClear(GL_DEPTH_BUFFER_BIT);

	// disable color writes (we only want to fill the Z-buffer)
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
}

void CShadowHandler::UnBindDepthTextureFBO() {
	if (!enabled)
		return;

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	// reset the viewport
	glViewport(
		gWindow->GetViewPort().pos.x,
		gWindow->GetViewPort().pos.y,
		gWindow->GetViewPort().size.x,
		gWindow->GetViewPort().size.y
	);

	// re-enable color write
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}



void CShadowHandler::DrawDepthTexture() {
	if (!enabled)
		return;

	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glTranslatef(-0.02f, -0.02f, 0.0f);

		glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);

		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texID);

		// draw in the top-right screen quadrant
		glBegin(GL_QUADS);
			glTexCoord2d(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
			glTexCoord2d(1.0f, 0.0f); glVertex2f(1.0f, 0.0f);
			glTexCoord2d(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);
			glTexCoord2d(0.0f, 1.0f); glVertex2f(0.0f, 1.0f);
		glEnd();
		glDisable(GL_TEXTURE_2D);

		glColor3f(1.0f, 0.0f, 0.0f);
		glBegin(GL_LINES);
			glVertex2f(0.0f, 0.0f);
			glVertex2f(1.0f, 0.0f);

			glVertex2f(1.0f, 0.0f);
			glVertex2f(1.0f, 1.0f);

			glVertex2f(1.0f, 1.0f);
			glVertex2f(0.0f, 1.0f);

			glVertex2f(0.0f, 1.0f);
			glVertex2f(0.0f, 0.0f);
		glEnd();

		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	glPopAttrib();
}
