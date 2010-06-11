#include <SDL/SDL.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include <cmath>
#include <cassert>
#include <iostream>

#include "../Input/InputHandler.hpp"
#include "../Sim/SimThread.hpp"
#include "../Renderer/RenderThread.hpp"
#include "../Renderer/CameraController.hpp"
#include "../Renderer/Camera.hpp"
#include "../System/EngineAux.hpp"
#include "../System/LuaParser.hpp"
#include "../System/Logger.hpp"
#include "../Math/vec3.hpp"
#include "../Math/Trig.hpp"
#include "../UI/Window.hpp"
#include "../UI/UI.hpp"
#include "./Client.hpp"
#include "./NetMessageBuffer.hpp"
#include "./ScopedTimer.hpp"

CClient* CClient::GetInstance(int argc, char** argv) {
	static CClient* c = NULL;
	static unsigned depth = 0;

	if (c == NULL) {
		assert(depth == 0);
		assert(argc != 0 && argv != NULL);

		depth += 1;
		c = new CClient(argc, argv);
		depth -= 1;
	}

	return c;
}

void CClient::FreeInstance(CClient* c) {
	delete c;
}



CClient::CClient(int argc, char** argv): clientID(0) {
	mInputHandler = CInputHandler::GetInstance();
	mInputHandler->AddReceiver(this);

	mSimThread = NULL;
	mRenderThread = NULL;
	mWindow = NULL;

	glutInit(&argc, argv);   // has to be called before Init()
	InitSDL();
	glewInit();              // has to be called after Init()

	// "pseudo-threads" (updated sequentially)
	// note: the map-loading code needs OpenGL
	mSimThread    = CSimThread::GetInstance();
	mRenderThread = CRenderThread::GetInstance();

	mUI = UI::GetInstance();
}

CClient::~CClient() {
	LOG << "[CClient::~CClient]\n";
	LOG << "\tcumulative [CClient::Update] time: "
		<< ScopedTimer::GetTaskTime("[CClient::Update]")
		<< "ms\n";

	UI::FreeInstance(mUI);
	SDLWindow::FreeInstance(mWindow);
	CRenderThread::FreeInstance(mRenderThread);
	CSimThread::FreeInstance(mSimThread);
	CInputHandler::FreeInstance(mInputHandler);

	SDL_Quit();
}



void CClient::Update() {
	ScopedTimer t("[CClient::Update]");

	ReadNetMessages();

	mInputHandler->Update();
	mRenderThread->Update();
	mWindow->Update();
	mUI->Update();

	SDL_GL_SwapBuffers();
}



void CClient::ReadNetMessages() {
	NetMessage m;

	while (mNetBuf->PopServerToClientMessage(&m))  {
		switch (m.GetID()) {
			case SERVER_MSG_SIMFRAME: {
				mSimThread->Update();

				NetMessage r(CLIENT_MSG_SIMFRAME, sizeof(unsigned int));
					r << clientID;
				SendNetMessage(r);
			} break;

			case CLIENT_MSG_SIMCOMMAND: {
				mSimThread->SimCommand(m);
			} break;

			default: {
				assert(false);
			} break;
		}
	}
}

void CClient::SendNetMessage(const NetMessage& m) {
	mNetBuf->AddClientToServerMessage(m);
}



void CClient::KeyPressed(int sdlKeyCode, bool repeat) {
	if (!repeat) {
		switch (sdlKeyCode) {
			case SDLK_ESCAPE: {
				SDL_Event e;
					e.type = SDL_QUIT;
					e.quit.type = SDL_QUIT;
				SDL_PushEvent(&e);
			} break;

			case SDLK_PAUSE: {
				SendNetMessage(NetMessage(CLIENT_MSG_PAUSE, 0));
			} break;

			case SDLK_EQUALS: {
				SendNetMessage(NetMessage(CLIENT_MSG_INCSIMSPEED, 0));
			} break;
			case SDLK_MINUS: {
				SendNetMessage(NetMessage(CLIENT_MSG_DECSIMSPEED, 0));
			} break;

			case SDLK_g: {
				const int grabMode = SDL_WM_GrabInput(SDL_GRAB_QUERY);

				if (grabMode == SDL_GRAB_ON ) { SDL_WM_GrabInput(SDL_GRAB_OFF); }
				if (grabMode == SDL_GRAB_OFF) { SDL_WM_GrabInput(SDL_GRAB_ON ); }
			} break;

			case SDLK_m: {
				// note: auto-{en, dis}able when switching to/from FPS mode?
				AUX->ToggleMouseLook();
				SDL_ShowCursor(AUX->GetMouseLook()? SDL_DISABLE: SDL_ENABLE);
			} break;

			case SDLK_n: {
				// switch camera mode
				mRenderThread->GetCamCon()->SwitchCams();
			} break;
		}
	}
}



void CClient::WindowResized(int nx, int ny) {
	mWindow->SetWindowSize(vec3i(nx, ny, 0));
}

void CClient::WindowExposed() {
	InitStencilBuffer();
	mWindow->UpdateViewPorts();
}



void CClient::InitStencilBuffer() {
	glClearStencil(0);
	glClear(GL_STENCIL_BUFFER_BIT); SDL_GL_SwapBuffers();
	glClear(GL_STENCIL_BUFFER_BIT); SDL_GL_SwapBuffers();
}

void CClient::InitSDL() {
	LOG << "[CClient::InitSDL]\n";

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
		LOG << "\tSDL initialization error " << SDL_GetError() << "\n";
		assert(false);
	}

	SDL_version hdr; SDL_VERSION(&hdr);
	SDL_version lib = *(SDL_Linked_Version());

	mWindow = SDLWindow::GetInstance();

	const std::string glVersion( (const char*) glGetString(GL_VERSION) );
	const std::string glVendor(  (const char*) glGetString(GL_VENDOR)  );
	const std::string glRenderer((const char*) glGetString(GL_RENDERER));

	LOG << "\tGL_VERSION:  " << glVersion  << "\n";
	LOG << "\tGL_VENDOR:   " << glVendor   << "\n";
	LOG << "\tGL_RENDERER: " << glRenderer << "\n";
	LOG << "\n";
	LOG << "\tcompiled major, minor, patch: " << hdr.major << ", " << hdr.minor << ", " << hdr.patch << "\n";
	LOG << "\tlinked major, minor, patch: " << lib.major << ", " << lib.minor << ", " << lib.patch << "\n";
}
