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
#include "./Client.hpp"
#include "./NetMessageBuffer.hpp"
#include "./ScopedTimer.hpp"
#include "./Debug.hpp"

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
}

CClient::~CClient() {
	LOG << "[CClient::~CClient]\n";
	LOG << "\tcumulative [CClient::Update] time: "
		<< ScopedTimer::GetTaskTime("[CClient::Update]")
		<< "ms\n";

	KillSDL();

	CRenderThread::FreeInstance(mRenderThread);
	CSimThread::FreeInstance(mSimThread);
	CInputHandler::FreeInstance(mInputHandler);
}



void CClient::Update() {
	// [1] ~550K updates/sec ==> [2]  ~200K updates/sec
	ScopedTimer t("[CClient::Update]");

	// [2] ~200K updates/sec ==> [3A]  ~190K updates/sec (PFFG_SERVER_NOTHREAD true)
	// [2] ~200K updates/sec ==> [3B]  ~300K updates/sec (PFFG_SERVER_NOTHREAD false) (?!)
	ReadNetMessages();

	// [3A] ~190K updates/sec ==> [4]  ~145K updates/sec
	mInputHandler->Update();
	// [4 ] ~145K updates/sec ==> [5]  ~280  updates/sec
	mRenderThread->Update();
	// [4 ] ~145K updates/sec ==> [6]  ~160 updates/sec (with UI-update, without buffer-swap)
	// [1 ] ~550K updates/sec ==> [7]  ~160 updates/sec (with UI-update, with buffer-swap)
	// [1 ] ~550K updates/sec ==> [8] ~2060 updates/sec (without UI-update, with buffer-swap)
	mWindow->Update();
}



void CClient::ReadNetMessages() {
	NetMessage m;

	while (mNetBuf->PopServerToClientMessage(&m))  {
		switch (m.GetMessageID()) {
			case SERVER_MSG_SIMFRAME: {
				mSimThread->Update();

				NetMessage r(CLIENT_MSG_SIMFRAME, clientID, sizeof(unsigned int));
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



void CClient::KeyPressed(int key, bool repeat) {
	if (!repeat) {
		switch (key) {
			case SDLK_ESCAPE: {
				SDL_Event e;
					e.type = SDL_QUIT;
					e.quit.type = SDL_QUIT;
				SDL_PushEvent(&e);
			} break;

			case SDLK_PAUSE: {
				SendNetMessage(NetMessage(CLIENT_MSG_PAUSE, clientID, 0));
			} break;

			case SDLK_EQUALS: {
				SendNetMessage(NetMessage(CLIENT_MSG_INCSIMSPEED, clientID, 0));
			} break;
			case SDLK_MINUS: {
				SendNetMessage(NetMessage(CLIENT_MSG_DECSIMSPEED, clientID, 0));
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

			case SDLK_c: {
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
	mWindow->UpdateViewPorts();
}



void CClient::InitSDL() {
	LOG << "[CClient::InitSDL]\n";

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
		LOG << "\tSDL initialization error " << SDL_GetError() << "\n";
		assert(false);
	}

	SDL_version hdr; SDL_VERSION(&hdr);
	SDL_version lib = *(SDL_Linked_Version());

	mWindow = ui::SDLWindow::GetInstance();

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

void CClient::KillSDL() {
	ui::SDLWindow::FreeInstance(mWindow);
	SDL_Quit();
}
