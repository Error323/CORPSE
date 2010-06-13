#include "../System/EngineAux.hpp"
#include "../System/LuaParser.hpp"
#include "../System/EventHandler.hpp"
#include "../System/NetMessages.hpp"
#include "../Ext/CallOutHandler.hpp"
#include "../Map/Ground.hpp"
#include "../Map/MapInfo.hpp"
#include "../Map/ReadMap.hpp"
#include "../Path/PathModule.hpp"
#include "./SimThread.hpp"
#include "./SimCommands.hpp"
#include "./SimObjectHandler.hpp"

CSimThread* CSimThread::GetInstance() {
	static CSimThread* st = NULL;
	static unsigned int depth = 0;

	if (st == NULL) {
		assert(depth == 0);

		depth += 1;
		st = new CSimThread();
		depth -= 1;
	}

	return st;
}

void CSimThread::FreeInstance(CSimThread* st) {
	delete st;
}



CSimThread::CSimThread(): frame(0) {
	const LuaTable* rootTable = LUA->GetRoot();
	const LuaTable* generalTable = rootTable->GetTblVal("general");
	const LuaTable* mapTable = rootTable->GetTblVal("map");

	mMapInfo = CMapInfo::GetInstance(mapTable->GetStrVal("smf", "map.smf"));
	mGround  = CGround::GetInstance();
	mReadMap = CReadMap::GetInstance(generalTable->GetStrVal("mapsDir", "data/maps/") + mapTable->GetStrVal("smf", "map.smf"));

	mPathModule = GetPathModuleInstance(CallOutHandler::GetInstance());
	mPathModule->SetPriority(123);
	eventHandler->AddReceiver(mPathModule);

	// create objects after path-module is loaded
	mSimObjectHandler = SimObjectHandler::GetInstance();
	mSimObjectHandler->AddObjects();
	// initialize module after the object-handler
	mPathModule->Init();
}

CSimThread::~CSimThread() {
	CReadMap::FreeInstance(mReadMap);
	CGround::FreeInstance(mGround);
	CMapInfo::FreeInstance(mMapInfo);

	mSimObjectHandler->DelObjects();
	mPathModule->Kill();
	// destroy objects before path-module is unloaded
	SimObjectHandler::FreeInstance(mSimObjectHandler);
	eventHandler->DelReceiver(mPathModule);

	CallOutHandler::FreeInstance((CallOutHandler*) mPathModule->GetCallOutHandler());
	FreePathModuleInstance(mPathModule);
}

void CSimThread::Update() {
	mSimObjectHandler->Update();
	mPathModule->Update();

	frame += 1;
}

void CSimThread::SimCommand(NetMessage& m) {
	unsigned int simCommandID = 0;
	unsigned int objectID     = 0;
	unsigned int objectDefID  = 0;
	vec3f objectPos;
	vec3f objectDir;

	m >> simCommandID;

	switch (simCommandID) {
		case COMMAND_CREATE_SIMOBJECT: {
			assert(!m.End()); m >> objectDefID;
			assert(!m.End()); m >> objectPos.x;
			assert(!m.End()); m >> objectPos.y;
			assert(!m.End()); m >> objectPos.z;
			assert(!m.End()); m >> objectDir.x;
			assert(!m.End()); m >> objectDir.y;
			assert(!m.End()); m >> objectDir.z;

			mSimObjectHandler->AddObject(objectDefID, objectPos, objectDir, false);
		} break;

		case COMMAND_DESTROY_SIMOBJECT: {
			assert(!m.End()); m >> objectID;

			mSimObjectHandler->DelObject(objectID, false);
		} break;

		case COMMAND_MOVE_SIMOBJECT: {
			SimObjectMoveOrderEvent e(frame);

			assert(!m.End()); m >> objectPos.x;
			assert(!m.End()); m >> objectPos.y;
			assert(!m.End()); m >> objectPos.z;
			assert(!m.End());

			e.SetGoalPos(objectPos);

			while (!m.End()) {
				m >> objectID;
				e.AddObjectID(objectID);

				assert(objectID < mSimObjectHandler->GetMaxSimObjects());
				assert(mSimObjectHandler->GetSimObject(objectID) != NULL);
			}

			eventHandler->NotifyReceivers(&e);
		} break;

		default: {
		} break;
	}
}
