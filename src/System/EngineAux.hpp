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
	bool GetLineSmoothing() const { return lineSmoothing; }
	bool GetPointSmoothing() const { return pointSmoothing; }

private:
	lua_State* luaState;

	LuaParser* luaParser;
	CLogger* logger;

	static int argc;
	static char** argv;
	static char cwd[1024];

	bool wantQuit;
	bool wantDraw;
	bool mouseLook;                 // if true/false, control FPS camera with mouse/keyboard

	bool lineSmoothing;
	bool pointSmoothing;
};

#define AUX EngineAux::GetInstance(0, NULL)
#define LUA (AUX->GetLuaParser())
#define LOG *(AUX->GetLogger())

#endif
