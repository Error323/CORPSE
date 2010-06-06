#include <cstdlib>
#include <iostream>

#include <lua5.1/lua.hpp>

#include "./EngineAux.hpp"
#include "./LuaParser.hpp"
#include "./Logger.hpp"

#include "Corpse.hpp"

int    EngineAux::argc      = 0;
char** EngineAux::argv      = NULL;
char   EngineAux::cwd[1024] = {'\0'};

EngineAux* EngineAux::GetInstance() {
	static EngineAux e;
	return &e;
}

void EngineAux::Init(int argCnt, char** argVec) {
	if (argCnt < 2) {
		std::cout << "[EngineAux::Init] usage: " << argVec[0] << " <params.lua>" << std::endl;
		exit(0);
	}

	srandom(time(0x0));
	getcwd(EngineAux::cwd, 1024);

	EngineAux::argc = argCnt;
	EngineAux::argv = argVec;
}



EngineAux::EngineAux() {
	luaState = lua_open();
	luaL_openlibs(luaState);

	luaParser = new LuaParser(luaState);

	if (!luaParser->Execute(EngineAux::argv[1], "params")) {
		std::cout << "[EngineAux::EngineAux]";
		std::cout << " error \"" << luaParser->GetError(EngineAux::argv[1]);
		std::cout << "\" while parsing " << EngineAux::argv[1];
		std::cout << std::endl;
		exit(0);
	}

	engState.Init(luaParser);
	winState.Init(luaParser);
	inpState.Init(luaParser);

	logger = new CLogger(luaParser);
	logger->Log("[EngineAux::EngineAux] current working-directory: " + std::string(cwd));
}

EngineAux::~EngineAux() {
	lua_close(luaState);

	delete luaParser; luaParser = 0x0;
	delete logger;    logger    = 0x0;
}



void EngineAux::EngineState::Init(LuaParser* p) {
	useFSAA   = bool(int(p->GetRoot()->GetTblVal("general")->GetFltVal("useFSAA", 1)));
	mouseLook = bool(int(p->GetRoot()->GetTblVal("general")->GetFltVal("mouseLook", 1)));

	fullScreen      = bool(int(p->GetRoot()->GetTblVal("general")->GetFltVal("fullScreen", 0)));
	dualScreen      = bool(int(p->GetRoot()->GetTblVal("general")->GetFltVal("dualScreen", 0)));
	lineSmoothing   = bool(int(p->GetRoot()->GetTblVal("general")->GetFltVal("lineSmoothing", 1)));
	pointSmoothing  = bool(int(p->GetRoot()->GetTblVal("general")->GetFltVal("pointSmoothing", 1)));
	bitsPerPixel    = unsigned(p->GetRoot()->GetTblVal("general")->GetFltVal("bitsPerPixel", 32));
	depthBufferBits = unsigned(p->GetRoot()->GetTblVal("general")->GetFltVal("depthBufferBits", 24));
	FSAALevel       = unsigned(p->GetRoot()->GetTblVal("general")->GetFltVal("FSAALevel", 4));
}

void EngineAux::InputState::Init(LuaParser* p) {
	const LuaTable* rootTable = p->GetRoot();
	const LuaTable* inputTable = rootTable->GetTblVal("input");

	inputFrameRate = inputTable->GetFltVal("inputRate", 100);
	inputFrameTime = 1000 / inputFrameRate;

	keySens   = inputFrameTime * inputTable->GetFltVal("keySens", 0.5f * 100.0f) * 0.01f;
	mouseSens = inputFrameTime * inputTable->GetFltVal("mouseSens", 0.2f * 100.0f) * 0.01f;

	currMouseX = lastMouseX = -1;
	currMouseY = lastMouseY = -1;

	lastMouseButton = -1;
	lastInputTick = 0;
}

void EngineAux::WindowState::Init(LuaParser* p) {
	const LuaTable* rootTable = p->GetRoot();
	const LuaTable* windowTable = rootTable->GetTblVal("window");
	const LuaTable* vportTable = rootTable->GetTblVal("viewport");

	desktopSizeX  = desktopSizeY  = 0;
	windowSizeX   = windowSizeY   = 0;
	windowPosX    = windowPosY    = 0;
	viewPortSizeX = viewPortSizeY = 0;
	viewPortPosX  = viewPortPosY  = 0;
	pixelSizeX    = pixelSizeY    = 0.0f;

	viewPortSizeX = vportTable->GetFltVal("xsize", 800);
	viewPortSizeY = vportTable->GetFltVal("ysize", 600);
	windowSizeX = windowTable->GetFltVal("xsize", 800);
	windowSizeY = windowTable->GetFltVal("ysize", 600);

	title = windowTable->GetStrVal("title", "") + " " + HUMAN_NAME;
}
