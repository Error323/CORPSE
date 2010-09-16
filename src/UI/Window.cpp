#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>
#include <GL/glxew.h>
#include <GL/gl.h>

#include "./Window.hpp"
#include "./UI.hpp"
#include "../System/EngineAux.hpp"
#include "../System/LuaParser.hpp"
#include "../System/Logger.hpp"
#include "../System/Debugger.hpp"

#include "CORPSE.hpp"

ui::IWindow* ui::IWindow::GetInstance() {
	static IWindow* w = NULL;
	static unsigned int depth = 0;

	if (w == NULL) {
		PFFG_ASSERT(depth == 0);

		depth += 1;
		w = new SDLWindow();
		depth -= 1;
	}

	return w;
}

void ui::IWindow::FreeInstance(IWindow* w) {
	delete w;
}



ui::SDLWindow::SDLWindow() {
	const LuaTable* rootTable = LUA->GetRoot();
	const LuaTable* windowTable = rootTable->GetTblVal("window");
	const LuaTable* vportTable = rootTable->GetTblVal("viewport");

	fullScreen      = bool(int(windowTable->GetFltVal("fullScreen", 0)));
	dualScreen      = bool(int(windowTable->GetFltVal("dualScreen", 0)));

	bitsPerPixel    = unsigned(windowTable->GetFltVal("bitsPerPixel", 32));
	depthBufferBits = unsigned(windowTable->GetFltVal("depthBufferBits", 24));
	useFSAA         = bool(unsigned(windowTable->GetFltVal("useFSAA", 1)));
	FSAALevel       = unsigned(windowTable->GetFltVal("FSAALevel", 4));

	icon = windowTable->GetStrVal("icon", "");
	title = windowTable->GetStrVal("title", "") + " " + HUMAN_NAME;

	SDL_WM_SetIcon(SDL_LoadBMP(icon.c_str()), NULL);
	SDL_WM_SetCaption(title.c_str(), NULL);

	SetWindowSize(vec3i(windowTable->GetFltVal("xsize", 800), windowTable->GetFltVal("ysize", 600), 0));
	AddViewPort();

	(viewPorts.front()).pos.x  = 0;
	(viewPorts.front()).pos.y  = 0;
	(viewPorts.front()).size.x = vportTable->GetFltVal("xsize", 800);
	(viewPorts.front()).size.y = vportTable->GetFltVal("ysize", 600);
	(viewPorts.front()).pxl.x  = 1.0f / (viewPorts.front()).size.x;
	(viewPorts.front()).pxl.y  = 1.0f / (viewPorts.front()).size.y;

	mUI = UI::GetInstance();
}

ui::SDLWindow::~SDLWindow() {
	ui::UI::FreeInstance(mUI);
}

void ui::SDLWindow::Update() {
	mUI->Update((viewPorts.front()).pos, (viewPorts.front()).size);
	SDL_GL_SwapBuffers();
}

void ui::SDLWindow::SetWindowSize(const vec3i& xy) {
	IWindow::SetWindowSize(xy);

	SetSDLVideoMode();
	UpdateGeometry();
	UpdateViewPorts();
}



void ui::SDLWindow::SetSDLVideoMode() {
	// set the video mode (32 bits per pixel, etc)
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,      (GetBitsPerPixel() >> 2));
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,    (GetBitsPerPixel() >> 2));
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,     (GetBitsPerPixel() >> 2));
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,    (GetBitsPerPixel() >> 2));

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,                     1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,    GetDepthBufferBits());
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,                     1);
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL,                     0);

	{
		#ifndef WIN32
		const GLubyte* glXSwapIntervalSGIs = reinterpret_cast<const GLubyte*>("glXSwapIntervalSGI");
		const PFNGLXSWAPINTERVALSGIPROC glXSwapIntervalSGI = reinterpret_cast<PFNGLXSWAPINTERVALSGIPROC>(glXGetProcAddress(glXSwapIntervalSGIs));

		// disable v-sync to try speeding up SDL_GL_SwapBuffers
		if (glXSwapIntervalSGI != NULL) {
			glXSwapIntervalSGI(0);
		}
		#endif
	}

	// this needs to be done prior to calling SetVideoMode()
	if (GetUseFSAA()) {
		SetUseFSAA(EnableMultiSampling()); SetFSAALevel((GetUseFSAA()? GetFSAALevel(): 0));
		SetUseFSAA(VerifyMultiSampling()); SetFSAALevel((GetUseFSAA()? GetFSAALevel(): 0));
	}

	mScreen = SDL_SetVideoMode(
		windowSize.x,
		windowSize.y,
		GetBitsPerPixel(),
		SDL_OPENGL |
		SDL_RESIZABLE |
		SDL_HWSURFACE |
		SDL_DOUBLEBUF |
		(GetFullScreen()? SDL_FULLSCREEN: 0)
	);

	if (mScreen == NULL) {
		LOG << "[SDLWindow::SetSDLVideoMode]\n";
		LOG << "\tSDL video mode error " << SDL_GetError() << "\n";
		fprintf(stderr, "SDL Error: %s\n", SDL_GetError());
		exit(1);
	}
}



void ui::SDLWindow::UpdateViewPorts() {
	int n = 0;

	for (std::list<ViewPort>::iterator it = viewPorts.begin(); it != viewPorts.end(); ++it) {
		if (!GetDualScreen()) {
			(*it).size = windowSize / viewPorts.size();
			(*it).pos = (*it).size * n;
			(*it).pxl.x = 1.0f / (*it).size.x;
			(*it).pxl.y = 1.0f / (*it).size.y;
		} else {
			// divide ports over either the left or the right screen-half
			if (GetDualScreenMapLeft()) {
			} else {
			}
		}

		n++;
	}

	glViewport(GetViewPort().pos.x, GetViewPort().pos.y, GetViewPort().size.x, GetViewPort().size.y);

	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();

	glClearStencil(0);
	// glClear(GL_STENCIL_BUFFER_BIT); SDL_GL_SwapBuffers();
	// glClear(GL_STENCIL_BUFFER_BIT); SDL_GL_SwapBuffers();
}



bool ui::SDLWindow::UpdateGeometry() {
	#ifndef WIN32
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);

	if (!SDL_GetWMInfo(&info)) {
		// we can't get information, so assume desktop
		// has dimensions <screenWidth, screenHeight>
		desktopSize = windowSize;
		windowPos = vec3i(0, 0, 0);

		return false;
	}

	info.info.x11.lock_func();

	Display* xdisplay = info.info.x11.display;
	Window xwindow = info.info.x11.window;

	XWindowAttributes xattrs;
	XGetWindowAttributes(xdisplay, xwindow, &xattrs);

	desktopSize.x = WidthOfScreen(xattrs.screen);
	desktopSize.y = HeightOfScreen(xattrs.screen);
	// note: why do we need this when SDL
	// tells us the new windowSize already
	// via WindowResized?
	windowSize.x = xattrs.width;
	windowSize.y = xattrs.height;

	int xp, yp;

	{
		Window tmp;
		XTranslateCoordinates(xdisplay, xwindow, xattrs.root, 0, 0, &xp, &yp, &tmp);
	}

	windowPos.x = xp;
	windowPos.y = desktopSize.y - windowSize.y - yp;
	return true;
	#else
	return false;
	#endif
}




bool ui::SDLWindow::EnableMultiSampling(void) {
	if (!GL_ARB_multisample) {
		return false;
	}

	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, GetFSAALevel());
	glEnable(GL_MULTISAMPLE);

	return true;
}

bool ui::SDLWindow::VerifyMultiSampling(void) {
	GLint buffers, samples;
	glGetIntegerv(GL_SAMPLE_BUFFERS_ARB, &buffers);
	glGetIntegerv(GL_SAMPLES_ARB, &samples);

	return (buffers > 0 && samples > 0);
}
