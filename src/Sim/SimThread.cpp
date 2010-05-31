#include "../System/EngineAux.hpp"
#include "../System/LuaParser.hpp"
#include "../System/EventHandler.hpp"
#include "../Ext/CallOutHandler.hpp"
#include "../Map/Ground.hpp"
#include "../Map/MapInfo.hpp"
#include "../Map/ReadMap.hpp"
#include "../Path/PathModule.hpp"
#include "./SimThread.hpp"

#include "./SimObjectHandler.hpp"

CSimThread* CSimThread::GetInstance() {
	static CSimThread* st = NULL;

	if (st == NULL) {
		st = new CSimThread();
	}

	return st;
}

void CSimThread::FreeInstance(CSimThread* st) {
	delete st;
}



CSimThread::CSimThread() {
	const LuaTable* rootTable = LUA->GetRoot();
	const LuaTable* generalTable = rootTable->GetTblVal("general");
	const LuaTable* mapTable = rootTable->GetTblVal("map");

	mMapInfo = CMapInfo::GetInstance(mapTable->GetStrVal("smf", "map.smf"));
	mGround  = CGround::GetInstance();
	mReadMap = CReadMap::GetInstance(generalTable->GetStrVal("mapsDir", "data/maps/") + mapTable->GetStrVal("smf", "map.smf"));

	mPathModule = GetPathModuleInstance(CallOutHandler::GetInstance());
	eventHandler->AddReceiver(mPathModule);
	// create objects after path-module is loaded
	mSimObjectHandler = SimObjectHandler::GetInstance();
	// initialize module after the object-handler
	mPathModule->Init();
}

CSimThread::~CSimThread() {
	CReadMap::FreeInstance(mReadMap);
	CGround::FreeInstance(mGround);
	CMapInfo::FreeInstance(mMapInfo);

	// destroy objects before path-module is unloaded
	SimObjectHandler::FreeInstance(mSimObjectHandler);

	mPathModule->Kill();
	eventHandler->DelReceiver(mPathModule);

	CallOutHandler::FreeInstance((CallOutHandler*) mPathModule->GetCallOutHandler());
	FreePathModuleInstance(mPathModule);
}

void CSimThread::Update() {
	mSimObjectHandler->Update();
	mPathModule->Update();
}
