#ifndef PFFG_WINDOW_HDR
#define PFFG_WINDOW_HDR

#include <string>
#include <list>

#include "../Math/vec3fwd.hpp"
#include "../Math/vec3.hpp"

class IWindow {
public:
	static IWindow* GetInstance();
	static void FreeInstance(IWindow*);

	virtual void Update() {}

	virtual const vec3i& GetWindowPos() const { return windowPos; }
	virtual const vec3i& GetWindowSize() const { return windowSize; }
	virtual const vec3i& GetDesktopSize() { return desktopSize; }
	virtual void SetWindowPos(const vec3i& xy) { windowPos = xy; }
	virtual void SetWindowSize(const vec3i& xy) { windowSize = xy; }
	virtual void SetDesktopSize(const vec3i& xy) { desktopSize = xy; }

	struct ViewPort {
		vec3i pos;  // top-left corner pixel, relative to window (viewPosXY)
		vec3i size; // dimensions in pixels, must be <= window size (viewSizeXY)
		vec3f pxl;  // size of one pixel in viewport coordinates, 1 / viewPortSizeXY
	};

	virtual void AddViewPort() { viewPorts.push_back(ViewPort()); }
	virtual void DelViewPort() { viewPorts.pop_back(); }

	virtual const ViewPort& GetViewPort() const { return viewPorts.front(); }
	virtual const std::list<ViewPort>& GetViewPorts() const { return viewPorts; }

	virtual void UpdateViewPorts() {}


	virtual bool GetFullScreen() const { return fullScreen; }
	virtual bool GetDualScreen() const { return dualScreen; }
	virtual bool GetDualScreenMapLeft() const { return dualScreenMapLeft; }
	virtual unsigned int GetBitsPerPixel() const { return bitsPerPixel; }
	virtual unsigned int GetDepthBufferBits() const { return depthBufferBits; }
	virtual unsigned int GetFSAALevel() const { return FSAALevel; }
	virtual bool GetUseFSAA() const { return useFSAA; }
	virtual void SetUseFSAA(bool b) { useFSAA = b; }
	virtual void SetFSAALevel(unsigned int n) { FSAALevel = n; }

	const std::string GetIcon() const { return icon; }
	const std::string GetTitle() const { return title; }

protected:
	virtual ~IWindow() {}

	std::list<ViewPort> viewPorts;

	vec3i desktopSize;            // dims of entire desktop in pixels (screen resolution, screenSizeXY)
	vec3i windowSize;             // dims of SDL/OGL window in pixels (resolution, winSizeXY)
	vec3i windowPos;              // xy-pos relative to bottom-left corner of desktop (winPosXY)
	// int screenWH;              // game screen width (REDUNDANT, winSizeXY)

	bool fullScreen;
	bool dualScreen;
	bool dualScreenMapLeft;       // dualScreen? "DualScreenMMapLeft": false;
	bool useFSAA;

	unsigned int bitsPerPixel;
	unsigned int depthBufferBits;
	unsigned int FSAALevel;       // anti-aliasing level; should be even number in [0, 8]

	std::string icon;
	std::string title;
};



struct SDL_Surface;
class SDLWindow: public IWindow {
public:
	SDLWindow();

	void Update() {}

	bool EnableMultiSampling();
	bool VerifyMultiSampling();

	void SetWindowSize(const vec3i&);
	void SetSDLWindowVideoMode();

	void UpdateViewPorts();

	// helper for UpdateViewPorts()
	bool UpdateWindowGeometry();

protected:
	~SDLWindow() {}

private:
	SDL_Surface* mScreen;
};

#define gWindow (SDLWindow::GetInstance())

#endif
