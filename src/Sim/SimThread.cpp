#include "../System/EngineAux.hpp"
#include "../System/LuaParser.hpp"
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

	mSimObjectHandler = SimObjectHandler::GetInstance();

	mPathModule = GetPathModuleInstance();
	mPathModule->Init();
}

CSimThread::~CSimThread() {
	SimObjectHandler::FreeInstance(mSimObjectHandler);

	CReadMap::FreeInstance(mReadMap);
	CGround::FreeInstance(mGround);
	CMapInfo::FreeInstance(mMapInfo);

	mPathModule->Kill();
	FreePathModuleInstance(mPathModule);
}

void CSimThread::Update() {
	mSimObjectHandler->Update();
	mPathModule->Update();
}
