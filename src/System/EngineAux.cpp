#include <cstdlib>
#include <iostream>

#include <lua5.1/lua.hpp>

#include "./EngineAux.hpp"
#include "./LuaParser.hpp"
#include "./Logger.hpp"

#include "CORPSE.hpp"

int    EngineAux::argc      = 0;
char** EngineAux::argv      = NULL;
char   EngineAux::cwd[1024] = {'\0'};

EngineAux* EngineAux::GetInstance(int argCnt, char** argVec) {
	static EngineAux* e = NULL;
	static unsigned int depth = 0;

	if (e == NULL) {
		assert(argCnt != 0 && argVec != NULL);
		assert(depth == 0);

		depth += 1;
		e = new EngineAux(argCnt, argVec);
		depth -= 1;
	}

	return e;
}

void EngineAux::FreeInstance(EngineAux* e) {
	delete e;
}



EngineAux::EngineAux(int argCnt, char** argVec) {
	if (argCnt < 2) {
		std::cout << "[EngineAux::Init] usage: " << argVec[0] << " <params.lua>" << std::endl;
		exit(0);
	}

	srandom(time(NULL));
	getcwd(EngineAux::cwd, 1024);

	EngineAux::argc = argCnt;
	EngineAux::argv = argVec;

	luaState = lua_open();
	luaL_openlibs(luaState);

	luaParser = new LuaParser(luaState);

	if (!luaParser->Execute(EngineAux::argv[1], "params")) {
		std::cout << "[EngineAux::EngineAux]";
		std::cout << " error \"" << luaParser->GetError(EngineAux::argv[1]);
		std::cout << "\" while parsing " << EngineAux::argv[1];
		std::cout << std::endl;
		exit(0);
	} else {
		const LuaTable* rootTable = luaParser->GetRoot();
		const LuaTable* generalTable = rootTable->GetTblVal("general");

		useFSAA   = bool(int(generalTable->GetFltVal("useFSAA", 1)));
		mouseLook = bool(int(generalTable->GetFltVal("mouseLook", 1)));

		wantDraw        = bool(int(generalTable->GetFltVal("wantDraw", 1)));
		wantQuit        = false;

		fullScreen      = bool(int(generalTable->GetFltVal("fullScreen", 0)));
		dualScreen      = bool(int(generalTable->GetFltVal("dualScreen", 0)));
		lineSmoothing   = bool(int(generalTable->GetFltVal("lineSmoothing", 1)));
		pointSmoothing  = bool(int(generalTable->GetFltVal("pointSmoothing", 1)));
		bitsPerPixel    = unsigned(generalTable->GetFltVal("bitsPerPixel", 32));
		depthBufferBits = unsigned(generalTable->GetFltVal("depthBufferBits", 24));
		FSAALevel       = unsigned(generalTable->GetFltVal("FSAALevel", 4));

		winState.Init(luaParser);

		logger = new CLogger(luaParser);
		logger->Log("[EngineAux::EngineAux] current working-directory: " + std::string(EngineAux::cwd));
	}
}

EngineAux::~EngineAux() {
	lua_close(luaState);

	delete luaParser; luaParser = NULL;
	delete logger;    logger    = NULL;
}



void EngineAux::WindowState::Init(LuaParser* p) {
	const LuaTable* rootTable = p->GetRoot();
	const LuaTable* windowTable = rootTable->GetTblVal("window");
	const LuaTable* vportTable = rootTable->GetTblVal("viewport");

	desktopSize.x =
	desktopSize.y = 0;

	windowPos.x   =
	windowPos.y   = 0;

	viewPortPos.x =
	viewPortPos.y = 0;

	pixelSize.x   =
	pixelSize.y   = 0.0f;

	viewPortSize.x = vportTable->GetFltVal("xsize", 800);
	viewPortSize.y = vportTable->GetFltVal("ysize", 600);
	windowSize.x   = windowTable->GetFltVal("xsize", 800);
	windowSize.y   = windowTable->GetFltVal("ysize", 600);

	title = windowTable->GetStrVal("title", "") + " " + HUMAN_NAME;
}
