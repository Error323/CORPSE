#include <cstdlib>
#include <iostream>

#include <lua5.1/lua.hpp>

#include "./EngineAux.hpp"
#include "./LuaParser.hpp"
#include "./Logger.hpp"

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
	assert(getcwd(EngineAux::cwd, 1024) != NULL);

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

		wantDraw        = true;
		wantQuit        = false;

		mouseLook       = bool(int(generalTable->GetFltVal("mouseLook", 1)));
		lineSmoothing   = bool(int(generalTable->GetFltVal("lineSmoothing", 1)));
		pointSmoothing  = bool(int(generalTable->GetFltVal("pointSmoothing", 1)));

		logger = new CLogger(luaParser);
		logger->Log("[EngineAux::EngineAux] current working-directory: " + std::string(EngineAux::cwd));
	}
}

EngineAux::~EngineAux() {
	lua_close(luaState);

	delete luaParser; luaParser = NULL;
	delete logger;    logger    = NULL;
}
