#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <SDL/SDL.h>

#include "../Map/MapInfo.hpp"
#include "./RenderThread.hpp"
#include "./Camera.hpp"
#include "./CameraController.hpp"
#include "./Scene.hpp"

#include "../Math/vec3fwd.hpp"
#include "../Math/vec3.hpp"
#include "../Math/Trig.hpp"
#include "../System/EngineAux.hpp"
#include "../System/Logger.hpp"

CRenderThread* CRenderThread::GetInstance() {
	static CRenderThread* rt = NULL;

	if (rt == NULL) {
		rt = new CRenderThread();
	}

	return rt;
}

void CRenderThread::FreeInstance(CRenderThread* rt) {
	delete rt;
}



CRenderThread::CRenderThread() {
	camCon = new CCameraController();
	scene = new CScene();

	InitLight();
}

CRenderThread::~CRenderThread() {
	delete camCon;
	delete scene;
}



inline static void PreFrameState() {
	glEnable(GL_BLEND);
	// glEnable(GL_NORMALIZE);

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
	glDepthRange(0.0f, 1.0f);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	// note: default winding is already CCW
	glFrontFace(GL_CCW);

	glPolygonMode(GL_FRONT, GL_FILL);
	glShadeModel(GL_SMOOTH);

	if (ENG->GetLineSmoothing()) {
		glEnable(GL_LINE_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	}
	if (ENG->GetPointSmoothing()) {
		glEnable(GL_POINT_SMOOTH);
		glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
	}

	// poor-man's FSAA (unused)
	// glEnable(GL_POLYGON_SMOOTH);

	if (ENG->GetUseFSAA()) {
		glEnable(GL_MULTISAMPLE_ARB);
	}


	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glClearDepth(1.0f);
	glClearStencil(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

inline static void PostFrameState() {
	if (ENG->GetUseFSAA()) {
		glDisable(GL_MULTISAMPLE_ARB);
	}

	// glDisable(GL_POLYGON_SMOOTH);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	// glDisable(GL_NORMALIZE);
	glDisable(GL_BLEND);

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
}

void CRenderThread::Update() {
	if (ENG->GetWantDraw()) {
		PreFrameState();

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
			glLoadIdentity();

			camCon->Update();
			scene->Draw(camCon->GetCurrCam());
		glPopMatrix();

		PostFrameState();
		SDL_GL_SwapBuffers();
	}
}



vec3f CRenderThread::MouseToWorldCoors(int mx, int my) {
	// these need to be doubles
	double wcoors[3] = {0.0, 0.0, 0.0};
	double viewMat[16] = {0.0};
	double projMat[16] = {0.0};
	int viewport[4] = {0};

	// todo: just use the current camera's matrices
	glGetDoublev(GL_PROJECTION_MATRIX, projMat);
	glGetDoublev(GL_MODELVIEW_MATRIX, viewMat);
	glGetIntegerv(GL_VIEWPORT, viewport);

	// note: mz value of 0 maps to zNear, mz value of 1 maps to zFar
	// mouse origin is at top-left, OGL window origin is at bottom-left
	float mz = 1.0f;
	int myy = viewport[3] - my;

	glReadPixels(mx, myy,  1, 1,  GL_DEPTH_COMPONENT, GL_FLOAT, &mz);
	gluUnProject(mx, myy, mz,  viewMat, projMat, viewport,  &wcoors[0], &wcoors[1], &wcoors[2]);

	return vec3f(float(wcoors[0]), float(wcoors[1]), float(wcoors[2]));
}



void CRenderThread::InitLight(void) {
	const GLfloat* ambientLightCol  = (GLfloat*) &mapInfo->light.groundAmbientColor.x;
	const GLfloat* diffuseLightCol  = (GLfloat*) &mapInfo->light.groundDiffuseColor.x;
	const GLfloat* specularLightCol = (GLfloat*) &mapInfo->light.groundSpecularColor.x;
	const GLfloat* lightDir         = (GLfloat*) &mapInfo->light.sunDir.x;

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
		glLoadIdentity();
		// if we multiply by the view-matrix here, the shaders
		// don't need to transform the dir with gl_NormalMatrix
		//
		// glMultMatrixf(camCon->GetCurrCam()->GetViewMatrix());
		glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLightCol);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLightCol);
		glLightfv(GL_LIGHT0, GL_SPECULAR, specularLightCol);
		glLightfv(GL_LIGHT0, GL_POSITION, lightDir);
	glPopMatrix();

	/*
	// GLfloat globalAmbientLight[] = {1.0f, 1.0f, 1.0f, 1.0f};
	// GLfloat specularReflection[] = {1.0f, 1.0f, 1.0f, 1.0f};
	// GLfloat materialEmission[] = {0.2f, 0.1f, 0.2f, 1.0f};
	//
	// disabling is required for the glMaterial() calls to work
	// glDisable(GL_COLOR_MATERIAL);
	// glMaterialfv(GL_FRONT, GL_EMISSION, materialEmission);
	// glMaterialfv(GL_FRONT, GL_SPECULAR, specularReflection);
	// glMateriali(GL_FRONT, GL_SHININESS, 128);

	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHTING);

	// glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientLight);
	// glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbientLight);
	// glLightModelfv(GL_LIGHT_MODEL_LOCAL_VIEWER, globalAmbientLight);

	// make the ambient (front)-material
	// properties track the current color
	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT, GL_AMBIENT);

	lightEnabled = true;
	*/
}
