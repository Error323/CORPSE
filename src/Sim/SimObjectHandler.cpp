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
	simObjectGridCells.resize(simObjects.size());

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
	simObjectGridCells.clear();

	mSimObjectDefHandler->DelDefs();
	SimObjectDefHandler::FreeInstance(mSimObjectDefHandler);

	SimObjectGrid<const SimObject*>::FreeInstance(mSimObjectGrid);
}

void SimObjectHandler::DelObjects() {
	while (!simObjectUsedIDs.empty()) {
		DelObject(simObjects[ *(simObjectUsedIDs.begin()) ], true);
	}
}

void SimObjectHandler::Update(unsigned int frame) {
	for (std::set<unsigned int>::const_iterator it = simObjectUsedIDs.begin(); it != simObjectUsedIDs.end(); ++it) {
		SimObject* o = simObjects[*it];

		const unsigned int objectID = o->GetID();
		const bool objectGridUpdate =
			(o->GetPhysicalState().currentForwardSpeed > 0.0f) ||
			(o->GetWantedPhysicalState().wantedForwardSpeed > 0.0f);

		if (objectGridUpdate) {
			mSimObjectGrid->DelObject(o, simObjectGridCells[objectID] );
		}

		o->Update();

		if (objectGridUpdate) {
			mSimObjectGrid->AddObject(o, simObjectGridCells[objectID] );
		}
	}

	CheckSimObjectCollisions(frame);
}



void SimObjectHandler::AddObject(unsigned int defID, const vec3f& pos, const vec3f& dir, bool inConstructor) {
	if (!simObjectFreeIDs.empty()) {
		vec3f gpos = pos;
			gpos.y = ground->GetHeight(pos.x, pos.z);
		mat44f mat = mat44f(gpos, NVECf, NVECf, dir);
			mat.SetYDirXZ(ground->GetSmoothNormal(gpos.x, gpos.z));
		WantedPhysicalState wps;
			wps.wantedPos = mat.GetPos();
			wps.wantedDir = mat.GetZDir();

		SimObjectDef* sod = mSimObjectDefHandler->GetDef(defID);
		SimObject* so = new SimObject(sod, *(simObjectFreeIDs.begin()));
			so->SetMat(mat);
			so->PushWantedPhysicalState(wps, false);

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

	mSimObjectGrid->AddObject(o, simObjectGridCells[o->GetID()] );

	SimObjectCreatedEvent e(((inConstructor)? 0: simThread->GetFrame()), o->GetID());
	eventHandler->NotifyReceivers(&e);
}

void SimObjectHandler::DelObject(SimObject* o, bool inDestructor) {
	assert(o != NULL);
	assert(simObjects[o->GetID()] == o);

	simObjectUsedIDs.erase(o->GetID());
	simObjectFreeIDs.insert(o->GetID());

	mSimObjectGrid->DelObject( o, simObjectGridCells[o->GetID()] );
	simObjectGridCells[o->GetID()].clear();

	SimObjectDestroyedEvent e(((inDestructor)? -1: simThread->GetFrame()), o->GetID());
	eventHandler->NotifyReceivers(&e);

	simObjects[o->GetID()] = NULL;
	delete o;
}



// get the single closest object within <radius> of <pos>
const SimObject* SimObjectHandler::GetClosestSimObject(const vec3f& pos, float radius) const {
	const SimObject* closestObject = NULL;

	std::list<const SimObject*> objects;
	mSimObjectGrid->GetObjects(pos, vec3f(radius, 0.0f, radius), objects);

	for (std::list<const SimObject*>::const_iterator it = objects.begin(); it != objects.end(); ++it) {
		const vec3f& objPos = (*it)->GetPos();
		const float objDst = (objPos - pos).sqLen3D();

		if (objDst > (radius * radius)) {
			continue;
		}

		if ((closestObject == NULL) || (objDst < (closestObject->GetPos() - pos).sqLen3D())) {
			closestObject = *it;
		}
	}

	return closestObject;
}



unsigned int SimObjectHandler::CheckSimObjectCollisions(unsigned int frame) {
	typedef const SimObject* Obj;
	typedef SimObjectGrid<Obj>::GridCell ObjCell;

	unsigned int numCollisions = 0;

	const std::list<ObjCell*>& cells = mSimObjectGrid->GetNonEmptyCells();

	for (std::list<ObjCell*>::const_iterator cit = cells.begin(); cit != cells.end(); ++cit) {
		const ObjCell* cell = *cit;
		const std::list<Obj> objects = cell->GetObjects();

		if (objects.size() == 1) {
			continue;
		}

		std::list<Obj>::const_iterator colliderIt = objects.begin();
		std::list<Obj>::const_iterator collideeIt;

		const SimObject* collider = NULL;
		const SimObject* collidee = NULL;

		float colliderRadius = 0.0f;
		float collideeRadius = 0.0f;

		float dstSq = 0.0f; // squared distance between positions
		float radSq = 0.0f; // squared radius of both spheres

		// within each cell, check only the unique pairs
		for (; colliderIt != objects.end(); ++colliderIt) {
			collideeIt = colliderIt; ++collideeIt;

			collider = *colliderIt;
			colliderRadius = collider->GetRadius();

			for (; collideeIt != objects.end(); ++collideeIt) {
				collidee = *collideeIt;
				collideeRadius = collidee->GetRadius();

				dstSq = (collider->GetPos() - collidee->GetPos()).sqLen3D();
				radSq = (colliderRadius + collideeRadius) * (colliderRadius + collideeRadius);

				if (dstSq < radSq) {
					numCollisions += 1;

					SimObjectCollisionEvent e(frame, collider->GetID(), collidee->GetID());
					eventHandler->NotifyReceivers(&e);
				}
			}
		}
	}

	return numCollisions;
}

void SimObjectHandler::PredictSimObjectCollisions(unsigned int numFrames) {
	static std::vector<PhysicalState> states(simObjects.size());

	// save the states
	for (std::set<unsigned int>::const_iterator it = simObjectUsedIDs.begin(); it != simObjectUsedIDs.end(); ++it) {
		states[*it] = simObjects[*it]->GetPhysicalState();
	}

	// advance the simulation
	for (unsigned int n = 0; n < numFrames; n++) {
		Update(simThread->GetFrame() + n);
	}

	// restore the states
	//
	// note that it is not necessary to "roll back" the object-grid, because
	//   1) if an object was moving, then the next regular update will re-add it at its old position
	//   2) if an object was not moving, then the prediction updates will not have moved it either
	for (std::set<unsigned int>::const_iterator it = simObjectUsedIDs.begin(); it != simObjectUsedIDs.end(); ++it) {
		simObjects[*it]->SetPhysicalState(states[*it]);
	}
}
