#include "../System/EngineAux.hpp"
#include "../System/LuaParser.hpp"
#include "../System/EventHandler.hpp"
#include "../System/NetMessages.hpp"
#include "../System/IEvent.hpp"
#include "../Ext/CallOutHandler.hpp"
#include "../Map/Ground.hpp"
#include "../Map/MapInfo.hpp"
#include "../Map/ReadMap.hpp"
#include "../Path/IPathModule.hpp"
#include "./SimThread.hpp"
#include "./SimCommands.hpp"
#include "./SimObjectHandler.hpp"

CSimThread* CSimThread::GetInstance() {
	static CSimThread* st = NULL;
	static unsigned int depth = 0;

	if (st == NULL) {
		PFFG_ASSERT(depth == 0);

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
	mSimObjectHandler->Update(frame);
	mPathModule->Update();

	frame += 1;
}

void CSimThread::SimCommand(NetMessage& m) {
	unsigned int simCommandID = 0;
	unsigned int objectID     = 0;
	unsigned int objectDefID  = 0;

	bool queueCommand = false;

	vec3f objectPos;
	vec3f objectDir;

	m >> simCommandID;

	switch (simCommandID) {
		case COMMAND_CREATE_SIMOBJECT: {
			PFFG_ASSERT(!m.End()); m >> objectDefID;
			PFFG_ASSERT(!m.End()); m >> objectPos.x;
			PFFG_ASSERT(!m.End()); m >> objectPos.y;
			PFFG_ASSERT(!m.End()); m >> objectPos.z;
			PFFG_ASSERT(!m.End()); m >> objectDir.x;
			PFFG_ASSERT(!m.End()); m >> objectDir.y;
			PFFG_ASSERT(!m.End()); m >> objectDir.z;

			if (readMap->PosInBounds(objectPos)) {
				mSimObjectHandler->AddObject(objectDefID, 0, objectPos, objectDir, false);
			}
		} break;

		case COMMAND_DESTROY_SIMOBJECT: {
			PFFG_ASSERT(!m.End()); m >> objectID;

			mSimObjectHandler->DelObject(objectID, false);
		} break;

		case COMMAND_MOVE_SIMOBJECT: {
			SimObjectMoveOrderEvent e(frame);

			PFFG_ASSERT(!m.End()); m >> objectPos.x;
			PFFG_ASSERT(!m.End()); m >> objectPos.y;
			PFFG_ASSERT(!m.End()); m >> objectPos.z;
			PFFG_ASSERT(!m.End()); m >> queueCommand;
			PFFG_ASSERT(!m.End());

			e.SetGoalPos(objectPos);
			e.SetQueued(queueCommand);

			while (!m.End()) {
				m >> objectID;

				if (mSimObjectHandler->IsValidSimObjectID(objectID)) {
					e.AddObjectID(objectID);
				}
			}

			eventHandler->NotifyReceivers(&e);
		} break;

		default: {
		} break;
	}
}
