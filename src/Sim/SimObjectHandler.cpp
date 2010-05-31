#include <cassert>

#include "./SimObjectHandler.hpp"
#include "./SimObject.hpp"
#include "./SimObjectDef.hpp"
#include "./SimObjectDefLoader.hpp"
#include "../Map/Ground.hpp"
#include "../System/EngineAux.hpp"
#include "../System/LuaParser.hpp"
#include "../System/IEvent.hpp"
#include "../System/EventHandler.hpp"

SimObjectHandler* SimObjectHandler::GetInstance() {
	static SimObjectHandler* soh = NULL;

	if (soh == NULL) {
		soh = new SimObjectHandler();
	}

	return soh;
}

void SimObjectHandler::FreeInstance(SimObjectHandler* soh) {
	delete soh;
}



SimObjectHandler::SimObjectHandler() {
	const LuaTable* rootTable = LUA->GetRoot();
	const LuaTable* objectsTable = rootTable->GetTblVal("objects");

	simObjects.resize(unsigned(objectsTable->GetFltVal("maxObjects", 10000)), NULL);

	for (unsigned int i = 0; i < simObjects.size(); i++) {
		simObjectFreeIDs.insert(i);
	}

	std::list<int> simObjectKeys;
	objectsTable->GetIntTblKeys(&simObjectKeys);

	for (std::list<int>::iterator it = simObjectKeys.begin(); it != simObjectKeys.end(); it++) {
		const LuaTable* objectTbl = objectsTable->GetTblVal(*it);

		vec3f pos = objectTbl->GetVec<vec3f>("pos", 3);
			pos.y = ground->GetHeight(pos.x, pos.z);
		mat44f mat = mat44f(pos, XVECf, YVECf, ZVECf);
			mat.SetYDirXZ(ground->GetNormal(pos.x, pos.z));

		SimObjectDef* sod = SimObjectDefLoader::GetDef(objectTbl->GetStrVal("mdl", ""));
		SimObject* so = new SimObject(sod, *(simObjectFreeIDs.begin()));
			so->SetMat(mat);

		AddObject(so);
	}
}

SimObjectHandler::~SimObjectHandler() {
	for (unsigned int i = 0; i < simObjects.size(); i++) {
		if (simObjects[i] != NULL) {
			DelObject(simObjects[i]);
		}
	}

	simObjectFreeIDs.clear();
	simObjectUsedIDs.clear();
	simObjects.clear();
}

void SimObjectHandler::Update() {
	for (std::set<unsigned int>::const_iterator it = simObjectUsedIDs.begin(); it != simObjectUsedIDs.end(); ++it) {
		simObjects[*it]->Update();
	}
}



void SimObjectHandler::AddObject(SimObject* o) {
	assert(simObjects[o->GetID()] == NULL);

	simObjects[o->GetID()] = o;
	simObjectUsedIDs.insert(o->GetID());
	simObjectFreeIDs.erase(o->GetID());

	SimObjectCreatedEvent e(123, o->GetID());
	eventHandler->NotifyReceivers(&e);
}

void SimObjectHandler::DelObject(SimObject* o) {
	assert(simObjects[o->GetID()] == o);

	simObjectUsedIDs.erase(o->GetID());
	simObjectFreeIDs.insert(o->GetID());

	SimObjectDestroyedEvent e(456, o->GetID());
	eventHandler->NotifyReceivers(&e);

	simObjects[o->GetID()] = NULL;
	delete o;
}
