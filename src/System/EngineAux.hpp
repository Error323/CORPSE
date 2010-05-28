#ifndef PFFG_ENGINEAUX_HDR
#define PFFG_ENGINEAUX_HDR

#include <cassert>

struct lua_State;
struct LuaParser;
class CLogger;

struct EngineAux {
public:
	EngineAux();
	~EngineAux();

	static void Init(int, char**);
	static EngineAux* GetInstance();

	LuaParser* GetLuaParser() { return luaParser; }
	CLogger* GetLogger() { return logger; }
	const char* GetCWD() const { return cwd; }

	struct EngineState;
	struct WindowState;
	struct InputState;
	EngineState* GetEngState() { return &engState; }
	WindowState* GetWinState() { return &winState; }
	InputState* GetInpState() { return &inpState; }


	struct EngineState {
	public:
		EngineState(): wantQuit(false), wantDraw(true) {}
		void Init(LuaParser*);

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

	private:
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


	struct InputState {
	public:
		void Init(LuaParser*);

		void SetLastInputTick(unsigned int t) { lastInputTick = t; }
		void SetCurrMouseCoors(int x, int y) { currMouseX = x; currMouseY = y; }
		void SetLastMouseCoors(int x, int y) { lastMouseX = x; lastMouseY = y; }
		void SetLastMouseButton(int b) { lastMouseButton = b; }

		int GetLastMouseX() const { return lastMouseX; }
		int GetLastMouseY() const { return lastMouseY; }
		int GetLastMouseButton() const { return lastMouseButton; }

		unsigned int GetLastInputTick() { return lastInputTick; }
		unsigned int GetInputFrameRate() { return inputFrameRate; }
		unsigned int GetInputFrameTime() { return inputFrameTime; }

		float GetKeySens() const { return keySens; }
		float GetMouseSens() const { return mouseSens; }

	private:
		int currMouseX, lastMouseX;
		int currMouseY, lastMouseY;
		int lastMouseButton;
		unsigned int lastInputTick;
		unsigned int inputFrameRate;
		unsigned int inputFrameTime;
		float keySens;
		float mouseSens;
	};


	struct WindowState {
	public:
		void Init(LuaParser*) {
			desktopSizeX  = desktopSizeY  = 0;
			windowSizeX   = windowSizeY   = 0;
			windowPosX    = windowPosY    = 0;
			viewPortSizeX = viewPortSizeY = 0;
			viewPortPosX  = viewPortPosY  = 0;
			pixelSizeX    = pixelSizeY    = 0.0f;
		}

		void SetWindowPosX(unsigned int x) { windowPosX = x; }
		void SetWindowPosY(unsigned int y) { windowPosY = y; }
		void SetWindowSizeX(unsigned int w) { windowSizeX = w; }
		void SetWindowSizeY(unsigned int h) { windowSizeY = h; }
		unsigned int GetWindowSizeX() { return windowSizeX; }
		unsigned int GetWindowSizeY() { return windowSizeY; }
		void SetViewPortPosX(unsigned int x) { viewPortPosX = x; }
		void SetViewPortPosY(unsigned int y) { viewPortPosY = y; }
		void SetViewPortSizeX(unsigned int w) { viewPortSizeX = w; }
		void SetViewPortSizeY(unsigned int h) { viewPortSizeY = h; }
		unsigned int GetViewPortPosX() { return viewPortPosX; }
		unsigned int GetViewPortPosY() { return viewPortPosY; }
		unsigned int GetViewPortSizeX() { return viewPortSizeX; }
		unsigned int GetViewPortSizeY() { return viewPortSizeY; }
		void SetDesktopSizeX(unsigned int w) { desktopSizeX = w; }
		void SetDesktopSizeY(unsigned int h) { desktopSizeY = h; }
		unsigned int GetDesktopSizeX() { return desktopSizeX; }
		unsigned int GetDesktopSizeY() { return desktopSizeY; }
		void SetPixelSizeX(float w) { pixelSizeX = w; }
		void SetPixelSizeY(float h) { pixelSizeY = h; }

	private:
		unsigned int desktopSizeX;      // width of entire desktop in pixels (screen resolution, screenSizeX)
		unsigned int desktopSizeY;      // height of entire desktop in pixels (screen resolution, screenSizeY)
		unsigned int windowSizeX;       // width of SDL/OGL window in pixels (resolution, winSizeX)
		unsigned int windowSizeY;       // height of SDL/OGL window in pixels (resolution, winSizeY)
		unsigned int windowPosX;        // x-pos relative to bottom-left corner of desktop (winPosX)
		unsigned int windowPosY;        // y-pos relative to bottom-left corner of desktop (winPosY)
		unsigned int viewPortSizeX;     // OGL viewport width in pixels (viewSizeX)
		unsigned int viewPortSizeY;     // OGL viewport height in pixels (viewSizeY)
		int viewPortPosX;               // x-pos relative to bottom-left corner of SDL/OGL window (viewPosX)
		int viewPortPosY;               // y-pos relative to bottom-left corner of SDL/OGL window (viewPosY)

		float pixelSizeX;               // width of one pixel in viewport coordinates, 1 / viewPortSizeX
		float pixelSizeY;               // height of one pixel in viewport coordinates, 1 / viewPortSizeY

		// int screenW;                 // game screen width (REDUNDANT, winSizeX)
		// int screenH;                 // game screen height (REDUNDANT, winSizeY)
	};

private:
	lua_State* luaState;

	LuaParser* luaParser;
	CLogger* logger;

	EngineState engState;
	WindowState winState;
	InputState inpState;

	static int argc;
	static char** argv;
	static char cwd[1024];
};

#define engAux EngineAux::GetInstance()
#define ENG (engAux->GetEngState())
#define INP (engAux->GetInpState())
#define WIN (engAux->GetWinState())
#define LUA (engAux->GetLuaParser())
#define LOG *(engAux->GetLogger())

#endif
