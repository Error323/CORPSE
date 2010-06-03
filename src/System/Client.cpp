#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>

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
#include "./Client.hpp"
#include "./NetMessageBuffer.hpp"
#include "./ScopedTimer.hpp"

CClient* CClient::GetInstance(int argc, char** argv) {
	static CClient* c = NULL;
	static unsigned depth = 0;

	if (c == NULL) {
		assert(depth == 0);

		depth += 1;
		c = new CClient(argc, argv);
		depth -= 1;
	}

	return c;
}

void CClient::FreeInstance(CClient* c) {
	delete c;
}



CClient::CClient(int argc, char** argv) {
	ParseArgs(argc, argv);

	mInputHandler = CInputHandler::GetInstance();
	mInputHandler->AddReceiver(this);

	mSimThread = NULL;
	mRenderThread = NULL;

	glutInit(&argc, argv);   // has to be called before Init()
	Init();
	glewInit();              // has to be called after Init()

	// "pseudo-threads" (updated sequentially)
	mSimThread    = CSimThread::GetInstance();
	mRenderThread = CRenderThread::GetInstance();
}

CClient::~CClient() {
	LOG << "[CClient::~CClient]\n";
	LOG << "\tcumulative [CClient::Update] time: "
		<< ScopedTimer::GetTaskTime("[CClient::Update]")
		<< "ms\n";

	CSimThread::FreeInstance(mSimThread);
	CRenderThread::FreeInstance(mRenderThread);
	CInputHandler::FreeInstance(mInputHandler);

	SDL_Quit();
}

void CClient::ParseArgs(int argc, char** argv) {
	bool resize = false;
	int  xres   = WIN->GetWindowSizeX();
	int  yres   = WIN->GetWindowSizeY();

	for (int i = 1; i < argc; i++) {
		std::string s(argv[i]);

		if (s.find("xres=") != std::string::npos && s.size() >= 6) {
			resize = true;
			xres   = atoi(&s[5]);
		}
		if (s.find("yres=") != std::string::npos && s.size() >= 6) {
			resize = true;
			yres   = atoi(&s[5]);
		}
	}

	if (resize) {
		SetWindowSize(xres, yres);
	}
}



void CClient::Init() {
	LOG << "[CClient::Init] [1]\n";

	const LuaTable* rootTable = LUA->GetRoot();
	const LuaTable* windowTable = rootTable->GetTblVal("window");

	const std::string& windowTitle = windowTable->GetStrVal("title", "");
	const unsigned int windowSizeX = windowTable->GetFltVal("xsize", 800);
	const unsigned int windowSizeY = windowTable->GetFltVal("ysize", 600);

	InitSDL(windowTitle.c_str());
	SetWindowSize(windowSizeX, windowSizeY);

	const std::string glVersion( (const char*) glGetString(GL_VERSION) );
	const std::string glVendor(  (const char*) glGetString(GL_VENDOR)  );
	const std::string glRenderer((const char*) glGetString(GL_RENDERER));

	LOG << "[CClient::Init] [2]\n";
	LOG << "\tGL_VERSION:  " << glVersion  << "\n";
	LOG << "\tGL_VENDOR:   " << glVendor   << "\n";
	LOG << "\tGL_RENDERER: " << glRenderer << "\n";
}



void CClient::Update() {
	ScopedTimer t("[CClient::Update]");

	ReadNetMessages();

	mInputHandler->Update();
	mRenderThread->Update();
}



void CClient::ReadNetMessages() {
	NetMessage m;

	while (netBuf->PopServerToClientMessage(&m))  {
		if (m.GetID() == SERVER_MSG_SIMFRAME) {
			mSimThread->Update();
		}
	}
}

void CClient::SendNetMessage(const NetMessage& m) {
	netBuf->AddClientToServerMessage(m);
}



void CClient::MouseMoved(int, int, int, int) {
	if (ENG->GetMouseLook()) {
		// re-center the mouse and eat the event it generates
		// note: this can also eat MouseReleased() events and
		// cause auto-move unless WE update the mouse state
		SDL_WarpMouse(mScreen->w >> 1, mScreen->h >> 1);
		SDL_Event e;
		while (SDL_PollEvent(&e)) {}
	}
}

void CClient::MousePressed(int button, int x, int y, bool repeat) {
	if (!repeat) {
		engAux->GetInpState()->SetLastMouseButton(button);
		engAux->GetInpState()->SetLastMouseCoors(x, y);
	}
}

void CClient::MouseReleased(int, int x, int y) {
	engAux->GetInpState()->SetLastMouseButton(-1);
	engAux->GetInpState()->SetLastMouseCoors(x, y);
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
				ENG->ToggleMouseLook();
				SDL_ShowCursor(ENG->GetMouseLook()? SDL_DISABLE: SDL_ENABLE);
			} break;

			case SDLK_n: {
				// switch camera mode
				mRenderThread->GetCamCon()->SwitchCams();
			} break;
		}
	}
}



void CClient::WindowResized(int nx, int ny, int gain) {
	switch (gain) {
		case -1: { SetWindowSize(nx, ny); } break;
		case  0: { ENG->SetWantDraw(false); } break;
		case  1: { ENG->SetWantDraw(true);  } break;
	}
}

void CClient::WindowExposed() {
	InitStencilBuffer();
	SetOGLViewPortGeometry();
}






void CClient::SetWindowSize(int w, int h) {
	WIN->SetWindowSizeX(w);
	WIN->SetWindowSizeY(h);

	SetSDLWindowVideoMode();
	SetOGLViewPortGeometry();
}

void CClient::InitStencilBuffer() {
	glClearStencil(0);
	glClear(GL_STENCIL_BUFFER_BIT); SDL_GL_SwapBuffers();
	glClear(GL_STENCIL_BUFFER_BIT); SDL_GL_SwapBuffers();
}



void CClient::InitSDL(const char* caption) {
	LOG << "[CClient::InitSDL]\n";

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
		LOG << "\tSDL initialization error " << SDL_GetError() << "\n";
		assert(false);
	}

	SDL_WM_SetIcon(SDL_LoadBMP("icon.bmp"), NULL);
	SDL_WM_SetCaption(caption, NULL);

	SDL_version hdr; SDL_VERSION(&hdr);
	SDL_version lib = *(SDL_Linked_Version());

	LOG << "\tcompiled major, minor, patch: " << hdr.major << ", " << hdr.minor << ", " << hdr.patch << "\n";
	LOG << "\tlinked major, minor, patch: " << lib.major << ", " << lib.minor << ", " << lib.patch << "\n";
}

// called on resize
void CClient::SetSDLWindowVideoMode() {
	// set the video mode (32 bits per pixel, etc)
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,      (ENG->GetBitsPerPixel() >> 2));
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,    (ENG->GetBitsPerPixel() >> 2));
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,     (ENG->GetBitsPerPixel() >> 2));
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,    (ENG->GetBitsPerPixel() >> 2));

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,                          1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,    ENG->GetDepthBufferBits());
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,                          1);

	// this needs to be done prior to calling SetVideoMode()
	if (ENG->GetUseFSAA()) {
		ENG->SetUseFSAA(EnableMultiSampling());
		ENG->SetFSAALevel((ENG->GetUseFSAA()? ENG->GetFSAALevel(): 0));
		ENG->SetUseFSAA(VerifyMultiSampling());
		ENG->SetFSAALevel((ENG->GetUseFSAA()? ENG->GetFSAALevel(): 0));
	}

	mScreen = SDL_SetVideoMode(
		WIN->GetWindowSizeX(),
		WIN->GetWindowSizeY(),
		ENG->GetBitsPerPixel(),
		SDL_OPENGL |
		SDL_RESIZABLE |
		SDL_HWSURFACE |
		SDL_DOUBLEBUF |
		(ENG->GetFullScreen()? SDL_FULLSCREEN: 0)
	);

	if (mScreen == NULL) {
		LOG << "[CClient::SetSDLWindowVideoMode]\n";
		LOG << "\tSDL video mode error " << SDL_GetError() << "\n";
		assert(false);
	}

	// this should not have to be done when resizing
	// InitStencilBuffer();

	// these should be equal to our bitsPerPixel value
	// SDL_GL_GetAttribute(SDL_GL_BUFFER_SIZE, &bits);
	// SDL_PixelFormat* f = mScreen->format;
}



// called on resize
void CClient::SetOGLViewPortGeometry() {
	UpdateViewPortDimensions();

	glViewport(
		WIN->GetViewPortPosX(),
		WIN->GetViewPortPosY(),
		WIN->GetViewPortSizeX(),
		WIN->GetViewPortSizeY()
	);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	if (mRenderThread) {
		// can't set up the projection matrix, must
		// delay until camera has been initialized
		glMultMatrixf((mRenderThread->GetCamCon()->GetCurrCam())->GetProjMatrix());
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}



// note: Linux-only
bool CClient::UpdateWindowInfo() {
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);

	if (!SDL_GetWMInfo(&info)) {
		// we can't get information, so assume desktop
		// has dimensions <screenWidth, screenHeight>
		WIN->SetDesktopSizeX(WIN->GetWindowSizeX());
		WIN->SetDesktopSizeY(WIN->GetWindowSizeY());

		WIN->SetWindowPosX(0);
		WIN->SetWindowPosY(0);
		return false;
	}

	info.info.x11.lock_func();

	Display* display = info.info.x11.display;
	Window window = info.info.x11.window;

	XWindowAttributes attrs;
	XGetWindowAttributes(display, window, &attrs);

	WIN->SetDesktopSizeX(WidthOfScreen(attrs.screen));
	WIN->SetDesktopSizeY(HeightOfScreen(attrs.screen));
	// note: why do we need this when SDL tells us
	// the new windowSizeX and windowSizeY already?
	WIN->SetWindowSizeX(attrs.width);
	WIN->SetWindowSizeY(attrs.height);

	Window tmp;
	int xp, yp;
	XTranslateCoordinates(display, window, attrs.root, 0, 0, &xp, &yp, &tmp);

	WIN->SetWindowPosX(xp);
	WIN->SetWindowPosY(WIN->GetDesktopSizeY() - WIN->GetWindowSizeY() - yp);

	return true;
}

void CClient::UpdateViewPortDimensions() {
	UpdateWindowInfo();

	if (!ENG->GetDualScreen()) {
		WIN->SetViewPortSizeX(WIN->GetWindowSizeX());
		WIN->SetViewPortSizeY(WIN->GetWindowSizeY());
		WIN->SetViewPortPosX(0);
		WIN->SetViewPortPosY(0);
	} else {
		WIN->SetViewPortSizeX(WIN->GetWindowSizeX() >> 1);
		WIN->SetViewPortSizeY(WIN->GetWindowSizeY() >> 0);

		if (ENG->GetDualScreenMapLeft()) {
			WIN->SetViewPortPosX(WIN->GetWindowSizeX() >> 1);
			WIN->SetViewPortPosY(0);
		} else {
			WIN->SetViewPortPosX(0);
			WIN->SetViewPortPosY(0);
		}
	}

	WIN->SetPixelSizeX(1.0f / float(WIN->GetViewPortSizeX()));
	WIN->SetPixelSizeY(1.0f / float(WIN->GetViewPortSizeY()));
}



bool CClient::EnableMultiSampling(void) {
	if (!GL_ARB_multisample) {
		return false;
	}

	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, ENG->GetFSAALevel());
	glEnable(GL_MULTISAMPLE);

	return true;
}

bool CClient::VerifyMultiSampling(void) {
	GLint buffers, samples;
	glGetIntegerv(GL_SAMPLE_BUFFERS_ARB, &buffers);
	glGetIntegerv(GL_SAMPLES_ARB, &samples);

	return (buffers > 0 && samples > 0);
}
