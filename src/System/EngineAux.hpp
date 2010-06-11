#ifndef PFFG_ENGINEAUX_HDR
#define PFFG_ENGINEAUX_HDR

#include <cassert>
#include <string>

#include "../Math/vec3fwd.hpp"
#include "../Math/vec3.hpp"

struct lua_State;
struct LuaParser;
class CLogger;

struct EngineAux {
public:
	EngineAux(int, char**);
	~EngineAux();

	static EngineAux* GetInstance(int, char**);
	static void FreeInstance(EngineAux*);

	LuaParser* GetLuaParser() { return luaParser; }
	CLogger* GetLogger() { return logger; }
	const char* GetCWD() const { return cwd; }

	bool GetWantQuit() const { return wantQuit; }
	void SetWantQuit(bool b) { wantQuit = b; }
	void SetWantDraw(bool b) { wantDraw = b; }
	bool GetWantDraw() const { return wantDraw; }
	bool GetMouseLook() const { return mouseLook; }
	void ToggleMouseLook() { mouseLook = !mouseLook; }
	bool GetFullScreen() const { return fullScreen; }
	bool GetDualScreen() const { return dualScreen; }
	bool GetDualScreenMapLeft() const { return dualScreenMapLeft; }
	unsigned int GetBitsPerPixel() const { return bitsPerPixel; }
	unsigned int GetDepthBufferBits() const { return depthBufferBits; }
	unsigned int GetFSAALevel() const { return FSAALevel; }
	bool GetUseFSAA() const { return useFSAA; }
	void SetUseFSAA(bool b) { useFSAA = b; }
	void SetFSAALevel(unsigned int n) { FSAALevel = n; }
	bool GetLineSmoothing() const { return lineSmoothing; }
	bool GetPointSmoothing() const { return pointSmoothing; }


	struct WindowState {
	public:
		void Init(LuaParser*);

		void SetViewPortPos(const vec3i& xy) { viewPortPos = xy; }
		void SetViewPortSize(const vec3i& xy) { viewPortSize = xy; }
		void SetWindowPos(const vec3i& xy) { windowPos = xy; }
		void SetWindowSize(const vec3i& xy) { windowSize = xy; }
		void SetDesktopSize(const vec3i& xy) { desktopSize = xy; }

		const vec3i& GetViewPortPos() const { return viewPortPos; }
		const vec3i& GetViewPortSize() const { return viewPortSize; }
		const vec3i& GetWindowPos() const { return windowPos; }
		const vec3i& GetWindowSize() const { return windowSize; }
		const vec3i& GetDesktopSize() { return desktopSize; }

		const vec3f& GetPixelSize() const { return pixelSize; }
		void SetPixelSize(const vec3f& xy) { pixelSize = xy; }

		const std::string GetTitle() const { return title; }

	private:
		vec3i desktopSize;      // dims of entire desktop in pixels (screen resolution, screenSizeXY)
		vec3i windowSize;       // dims of SDL/OGL window in pixels (resolution, winSizeXY)
		vec3i windowPos;        // xy-pos relative to bottom-left corner of desktop (winPosXY)
		vec3i viewPortSize;     // OGL viewport dims in pixels (viewSizeXY)
		vec3i viewPortPos;      // xy-pos relative to bottom-left corner of SDL/OGL window (viewPosXY)

		vec3f pixelSize;        // dims of one pixel in viewport coordinates, 1 / viewPortSizeXY

		// int screenWH;        // game screen width (REDUNDANT, winSizeXY)

		std::string title;
	};

	WindowState* GetWinState() { return &winState; }

private:
	lua_State* luaState;

	LuaParser* luaParser;
	CLogger* logger;

	WindowState winState;

	static int argc;
	static char** argv;
	static char cwd[1024];

	bool wantQuit;
	bool wantDraw;

	bool useFSAA;
	bool mouseLook;                 // if true/false, control FPS camera with mouse/keyboard

	bool fullScreen;
	bool dualScreen;
	bool dualScreenMapLeft;         // dualScreen? "DualScreenMMapLeft": false;
	bool lineSmoothing;
	bool pointSmoothing;

	unsigned int bitsPerPixel;
	unsigned int depthBufferBits;
	unsigned int FSAALevel;         // anti-aliasing level; should be even number in [0, 8]
};

#define AUX EngineAux::GetInstance(0, NULL)
#define WIN (AUX->GetWinState())
#define LUA (AUX->GetLuaParser())
#define LOG *(AUX->GetLogger())

#endif
