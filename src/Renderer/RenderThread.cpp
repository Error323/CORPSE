#include <GL/gl.h>
#include <SDL/SDL.h>

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
	static unsigned int depth = 0;

	if (rt == NULL) {
		assert(depth == 0);

		depth += 1;
		rt = new CRenderThread();
		depth -= 1;
	}

	return rt;
}

void CRenderThread::FreeInstance(CRenderThread* rt) {
	delete rt;
}



CRenderThread::CRenderThread(): frame(0) {
	camCon = new CCameraController();
	scene = new CScene();
}

CRenderThread::~CRenderThread() {
	delete camCon;
	delete scene;
}



inline static void PreFrameState() {
	static int preDL = 0;

	if (preDL == 0) {
		preDL = glGenLists(1);
		glNewList(preDL, GL_COMPILE);

		glEnable(GL_BLEND);

		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LESS);
		glDepthRange(0.0f, 1.0f);

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		// note: default winding is already CCW
		glFrontFace(GL_CCW);

		glPolygonMode(GL_FRONT, GL_FILL);

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

		glEndList();
	} else {
		glCallList(preDL);
	}
}

inline static void PostFrameState() {
	static int postDL = 0;

	if (postDL == 0) {
		postDL = glGenLists(1);
		glNewList(postDL, GL_COMPILE);

		if (ENG->GetUseFSAA()) {
			glDisable(GL_MULTISAMPLE_ARB);
		}

		// glDisable(GL_POLYGON_SMOOTH);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);

		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);

		glEndList();
	} else {
		glCallList(postDL);
	}
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

		frame += 1;
	}
}
