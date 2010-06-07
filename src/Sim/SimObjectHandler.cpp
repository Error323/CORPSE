#include <cassert>

#include "./SimObjectHandler.hpp"
#include "./SimObject.hpp"
#include "./SimObjectDef.hpp"
#include "./SimObjectDefHandler.hpp"
#include "./SimObjectGrid.hpp"
#include "./SimThread.hpp"
#include "../Map/Ground.hpp"
#include "../Map/ReadMap.hpp"
#include "../System/EngineAux.hpp"
#include "../System/LuaParser.hpp"
#include "../System/IEvent.hpp"
#include "../System/EventHandler.hpp"

SimObjectHandler* SimObjectHandler::GetInstance() {
	static SimObjectHandler* soh = NULL;
	static unsigned int depth = 0;

	if (soh == NULL) {
		assert(depth == 0);

		depth += 1;
		soh = new SimObjectHandler();
		depth -= 1;
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
	simObjectGridIts.resize(simObjects.size());

	for (unsigned int i = 0; i < simObjects.size(); i++) {
		simObjectFreeIDs.insert(i);
	}

	mSimObjectDefHandler = SimObjectDefHandler::GetInstance();

	// vertical dimension of grid must cover all y-values
	// that simulation objects can realistically exist at
	const vec3i& numObjectGridCells = objectsTable->GetVec<vec3i>("numObjectGridCells", 3);
	const vec3f  objectGridMins = vec3f(                                0.0f, -1e6f,                                 0.0f);
	const vec3f  objectGridMaxs = vec3f(readMap->mapx * readMap->SQUARE_SIZE,  1e6f, readMap->mapy * readMap->SQUARE_SIZE);

	mSimObjectGrid = SimObjectGrid<const SimObject*>::GetInstance(numObjectGridCells, objectGridMins, objectGridMaxs);
}

void SimObjectHandler::AddObjects() {
	const LuaTable* rootTable = LUA->GetRoot();
	const LuaTable* objectsTable = rootTable->GetTblVal("objects");

	if (mSimObjectDefHandler->LoadDefs()) {
		std::list<int> simObjectKeys;
		objectsTable->GetIntTblKeys(&simObjectKeys);

		for (std::list<int>::iterator it = simObjectKeys.begin(); it != simObjectKeys.end(); it++) {
			const LuaTable* objectTable = objectsTable->GetTblVal(*it);

			const SimObjectDef* def = mSimObjectDefHandler->GetDef(objectTable->GetStrVal("def", ""));
			const vec3f& pos = objectTable->GetVec<vec3f>("pos", 3);
			const vec3f& dir = objectTable->GetVec<vec3f>("dir", 3);

			AddObject(def->GetID(), pos, dir, true);
		}
	}
}

SimObjectHandler::~SimObjectHandler() {
	simObjectFreeIDs.clear();
	simObjectUsedIDs.clear();
	simObjects.clear();
	simObjectGridIts.clear();

	mSimObjectDefHandler->DelDefs();
	SimObjectDefHandler::FreeInstance(mSimObjectDefHandler);

	SimObjectGrid<const SimObject*>::FreeInstance(mSimObjectGrid);
}

void SimObjectHandler::DelObjects() {
	while (!simObjectUsedIDs.empty()) {
		DelObject(simObjects[ *(simObjectUsedIDs.begin()) ], true);
	}
}

void SimObjectHandler::Update() {
	for (std::set<unsigned int>::const_iterator it = simObjectUsedIDs.begin(); it != simObjectUsedIDs.end(); ++it) {
		SimObject* o = simObjects[*it];

		mSimObjectGrid->DelObject( o, simObjectGridIts[o->GetID()] );
		o->Update();
		simObjectGridIts[o->GetID()] = mSimObjectGrid->AddObject(o);
	}
}



void SimObjectHandler::AddObject(unsigned int defID, const vec3f& pos, const vec3f& dir, bool inConstructor) {
	if (!simObjectFreeIDs.empty()) {
		vec3f gpos = pos;
			gpos.y = ground->GetHeight(pos.x, pos.z);
		mat44f mat = mat44f(gpos, NVECf, NVECf, dir);
			mat.SetYDirXZ(ground->GetSmoothNormal(gpos.x, gpos.z));

		SimObjectDef* sod = mSimObjectDefHandler->GetDef(defID);
		SimObject* so = new SimObject(sod, *(simObjectFreeIDs.begin()));
			so->SetMat(mat);
			so->SetWantedDirection(mat.GetZDir());

		AddObject(so, inConstructor);
	}
}

void SimObjectHandler::DelObject(unsigned int objID, bool inDestructor) {
	if (!simObjectUsedIDs.empty()) {
		DelObject(simObjects[objID], inDestructor);
	}
}

void SimObjectHandler::AddObject(SimObject* o, bool inConstructor) {
	assert(o != NULL);
	assert(simObjects[o->GetID()] == NULL);

	simObjects[o->GetID()] = o;
	simObjectUsedIDs.insert(o->GetID());
	simObjectFreeIDs.erase(o->GetID());

	simObjectGridIts[o->GetID()] = mSimObjectGrid->AddObject(o);

	SimObjectCreatedEvent e(((inConstructor)? 0: simThread->GetFrame()), o->GetID());
	eventHandler->NotifyReceivers(&e);
}

void SimObjectHandler::DelObject(SimObject* o, bool inDestructor) {
	assert(o != NULL);
	assert(simObjects[o->GetID()] == o);

	simObjectUsedIDs.erase(o->GetID());
	simObjectFreeIDs.insert(o->GetID());

	mSimObjectGrid->DelObject( o, simObjectGridIts[o->GetID()] );
	// simObjectGridIts[o->GetID()] = NULL;

	SimObjectDestroyedEvent e(((inDestructor)? -1: simThread->GetFrame()), o->GetID());
	eventHandler->NotifyReceivers(&e);

	simObjects[o->GetID()] = NULL;
	delete o;
}
