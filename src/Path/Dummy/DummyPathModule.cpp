#include <iostream>

#include "./DummyPathModule.hpp"
#include "./FlowGrid.hpp"
#include "../../Math/vec3.hpp"
#include "../../Ext/ICallOutHandler.hpp"
#include "../../Sim/SimObjectDef.hpp"
#include "../../Sim/SimObjectState.hpp"

#define FLOWGRID_ENABLED 0
#define FLOWGRID_STEPS   1

void DummyPathModule::OnEvent(const IEvent* e) {
	switch (e->GetType()) {
		case EVENT_SIMOBJECT_CREATED: {
			const SimObjectCreatedEvent* ee = dynamic_cast<const SimObjectCreatedEvent*>(e);
			const unsigned int objectID = ee->GetObjectID();

			mObjects[objectID] = new MObject(coh->GetSimObjectDef(objectID));
		} break;

		case EVENT_SIMOBJECT_DESTROYED: {
			const SimObjectDestroyedEvent* ee = dynamic_cast<const SimObjectDestroyedEvent*>(e);
			const unsigned int objectID = ee->GetObjectID();

			DelObjectFromGroup(objectID);
			mObjects.erase(objectID);

			if (mObjects.empty()) {
				PFFG_ASSERT(mGroups.empty());

				// reset the group counter
				numGroupIDs = 0;
			}
		} break;

		case EVENT_SIMOBJECT_MOVEORDER: {
			const SimObjectMoveOrderEvent* ee = dynamic_cast<const SimObjectMoveOrderEvent*>(e);

			const std::list<unsigned int>& objectIDs = ee->GetObjectIDs();
			const vec3f& goalPos = ee->GetGoalPos();

			// create a new group
			const unsigned int groupID = numGroupIDs++;

			MGroup* newGroup = new MGroup();
			mGroups[groupID] = newGroup;

			for (std::list<unsigned int>::const_iterator it = objectIDs.begin(); it != objectIDs.end(); ++it) {
				const unsigned int objID = *it;
				const vec3f& objPos = coh->GetSimObjectPosition(objID);

				PFFG_ASSERT(coh->IsValidSimObjectID(objID));

				// note: direction is based on our current position
				WantedPhysicalState wps = coh->GetSimObjectWantedPhysicalState(objID, true);
					wps.wantedPos   = goalPos;
					wps.wantedDir   = (goalPos - objPos).norm3D();
					wps.wantedSpeed = mObjects[objID]->GetDef()->GetMaxForwardSpeed();

				coh->PushSimObjectWantedPhysicalState(objID, wps, ee->GetQueued(), false);

				DelObjectFromGroup(objID);
				AddObjectToGroup(groupID, objID);
			}
		} break;

		case EVENT_SIMOBJECT_COLLISION: {
			const SimObjectCollisionEvent* ee = dynamic_cast<const SimObjectCollisionEvent*>(e);

			const unsigned int colliderID = ee->GetColliderID();
			const unsigned int collideeID = ee->GetCollideeID();

			const vec3f& colliderPos = coh->GetSimObjectPosition(colliderID);
			const vec3f& collideePos = coh->GetSimObjectPosition(collideeID);

			const float colliderRadius = coh->GetSimObjectModelRadius(colliderID);
			const float collideeRadius = coh->GetSimObjectModelRadius(collideeID);

			const vec3f separationVec = colliderPos - collideePos;
			const float separationMin = (colliderRadius + collideeRadius) * (colliderRadius + collideeRadius);

			// enforce minimum distance between objects
			if ((separationVec.sqLen3D() - separationMin) < 0.0f) {
				const float dst = (separationVec.len3D());
				const vec3f dir = (separationVec / dst);
				const vec3f dif = (dir * (((colliderRadius + collideeRadius) - dst) * 0.5f));

				coh->SetSimObjectRawPosition(colliderID, colliderPos + dif);
				coh->SetSimObjectRawPosition(collideeID, collideePos - dif);
			}
		} break;

		default: {
		} break;
	}
}

void DummyPathModule::Init() {
	std::cout << "[DummyPathModule::Init]" << std::endl;

	#if (FLOWGRID_ENABLED == 1)
	flowGrid = new FlowGrid(coh);
	#endif
}

void DummyPathModule::Update() {
	#if (FLOWGRID_ENABLED == 1)
	flowGrid->Reset();

	for (ObjectMapIt it = mObjects.begin(); it != mObjects.end(); ++it) {
		flowGrid->AddFlow(it->first, FLOWGRID_STEPS);
	}

	flowGrid->AvgFlow();
	#endif

	// steer the sim-objects around the map based on user commands
	for (ObjectMapIt it = mObjects.begin(); it != mObjects.end(); ++it) {
		const MObject* obj = it->second;
		const unsigned int objID = it->first;
		const SimObjectDef* objDef = obj->GetDef();

		// get the front-most wanted state
		WantedPhysicalState wps = coh->GetSimObjectWantedPhysicalState(objID, true);

		const vec3f& pos = coh->GetSimObjectPosition(objID);
		const vec3f  vec = wps.wantedPos - pos;
		const float  dst = vec.sqLen3D();

		const float brakeTime = coh->GetSimObjectSpeed(objID) / objDef->GetMaxDeccelerationRate();
		const float brakeDist = 0.5f * objDef->GetMaxDeccelerationRate() * (brakeTime * brakeTime);

		// if only one waypoint left in queue:
		//    set wanted speed to 0 and replace the front of the queue
		// otherwise:
		//    pop front waypoint in queue
		//    update wanted direction of new front waypoint
		//
		if (coh->GetSimObjectNumWantedPhysicalStates(objID) <= 1) {
			if ((brakeDist * brakeDist) >= dst) {
				wps.wantedSpeed = 0.0f;
			} else {
				wps.wantedDir = vec / dst;
			}
		} else {
			if ((brakeDist * brakeDist) >= dst) {
				coh->PopSimObjectWantedPhysicalStates(objID, 1, true);
			}

			// peek at the next wanted state
			const WantedPhysicalState& nwps = coh->GetSimObjectWantedPhysicalState(objID, true);

			wps.wantedPos   = (nwps.wantedPos);
			wps.wantedDir   = (nwps.wantedPos - pos).norm3D();
			wps.wantedSpeed = nwps.wantedSpeed;
		}

		coh->PopSimObjectWantedPhysicalStates(objID, 1, true);
		coh->PushSimObjectWantedPhysicalState(objID, wps, true, true);
	}
}

void DummyPathModule::Kill() {
	std::cout << "[DummyPathModule::Kill]" << std::endl;

	for (GroupMapIt it = mGroups.begin(); it != mGroups.end(); ++it) {
		delete it->second;
	}
	for (ObjectMapIt it = mObjects.begin(); it != mObjects.end(); ++it) {
		delete it->second;
	}

	mGroups.clear();
	mObjects.clear();

	#if (FLOWGRID_ENABLED == 1)
	delete flowGrid;
	#endif
}



bool DummyPathModule::DelObjectFromGroup(unsigned int objectID) {
	MObject* object = mObjects[objectID]; // already created
	MGroup* group = NULL;

	PFFG_ASSERT(object != NULL);

	GroupMapIt git = mGroups.find(object->GetGroupID());

	if (git != mGroups.end()) {
		const unsigned int groupID = object->GetGroupID();

		group = (git->second);
		group->DelObject(objectID);

		if (group->IsEmpty()) {
			// old group is now empty, delete it
			mGroups.erase(groupID);
			delete group;
		}

		object->SetGroupID(-1);
		return true;
	}

	// object was not in a group
	return false;
}

void DummyPathModule::AddObjectToGroup(unsigned int groupID, unsigned int objectID) {
	MGroup* group = mGroups[groupID]; // already created
	MObject* object = mObjects[objectID];

	PFFG_ASSERT(group != NULL && object != NULL);

	group->AddObject(objectID);
	object->SetGroupID(groupID);
}

bool DummyPathModule::DelGroup(unsigned int groupID) {
	GroupMapIt git = mGroups.find(groupID);

	if (git == mGroups.end()) {
		return false;
	}

	const MGroup* group = git->second;
	const std::set<unsigned int>& gObjectIDs = group->GetObjectIDs();

	for (std::set<unsigned int>::const_iterator git = gObjectIDs.begin(); git != gObjectIDs.end(); ++git) {
		mObjects[*git]->SetGroupID(-1);
	}

	mGroups.erase(groupID);
	delete group;
	return true;
}
