#include <cassert>
#include <iostream>

#include "./PathModule.hpp"
#include "../Math/vec3.hpp"
#include "../Ext/ICallOutHandler.hpp"
#include "../Sim/SimObjectDef.hpp"
#include "../Sim/SimObjectState.hpp"

void PathModule::OnEvent(const IEvent* e) {
	switch (e->GetType()) {
		case EVENT_SIMOBJECT_CREATED: {
			const SimObjectCreatedEvent* ee = dynamic_cast<const SimObjectCreatedEvent*>(e);
			const unsigned int objectID = ee->GetObjectID();

			simObjectIDs[objectID] = coh->GetSimObjectDef(objectID);
		} break;
		case EVENT_SIMOBJECT_DESTROYED: {
			const SimObjectDestroyedEvent* ee = dynamic_cast<const SimObjectDestroyedEvent*>(e);
			const unsigned int objectID = ee->GetObjectID();

			simObjectIDs.erase(objectID);
			DelObjectFromGroup(objectID);

			if (simObjectIDs.empty()) {
				assert(objectGroupIDs.empty());
				assert(objectGroups.empty());

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

			for (std::list<unsigned int>::const_iterator it = objectIDs.begin(); it != objectIDs.end(); ++it) {
				const unsigned int objID = *it;

				assert(coh->IsValidSimObjectID(objID));

				// note: direction is based on our current position
				WantedPhysicalState wps = coh->GetSimObjectWantedPhysicalState(objID);
					wps.wantedPos = goalPos;
					wps.wantedDir = (goalPos - coh->GetSimObjectPosition(objID)).norm();
					wps.wantedForwardSpeed = simObjectIDs[objID]->GetMaxForwardSpeed();
				coh->SetSimObjectWantedPhysicalState(objID, wps, ee->GetQueued());

				DelObjectFromGroup(objID);
				AddObjectToGroup(objID, groupID);
			}
		} break;

		case EVENT_SIMOBJECT_COLLISION: {
			/*
			const SimObjectCollisionEvent* ee = dynamic_cast<const SimObjectCollisionEvent*>(e);

			const unsigned int colliderID = ee->GetColliderID();
			const unsigned int collideeID = ee->GetCollideeID();

			// TODO: react to this more intelligently
			coh->SetSimObjectWantedForwardSpeed(colliderID, 0.0f);
			coh->SetSimObjectWantedForwardSpeed(collideeID, 0.0f);
			*/
		} break;

		default: {
		} break;
	}
}

void PathModule::Init() {
	std::cout << "[PathModule::Init]" << std::endl;
}

void PathModule::Update() {
	// steer the sim-objects around the map based on user commands
	for (std::map<unsigned int, const SimObjectDef*>::const_iterator it = simObjectIDs.begin(); it != simObjectIDs.end(); ++it) {
		const unsigned int objID = it->first;
		const SimObjectDef* objDef = it->second;

		WantedPhysicalState wps = coh->GetSimObjectWantedPhysicalState(objID);

		const vec3f vec = wps.wantedPos - coh->GetSimObjectPosition(objID);
		const float dst = vec.sqLen3D();

		const float brakeTime = coh->GetSimObjectCurrentForwardSpeed(objID) / objDef->GetMaxDeccelerationRate();
		const float brakeDist = coh->GetSimObjectCurrentForwardSpeed(objID) * brakeTime; // conservative

		if ((brakeDist * brakeDist) >= dst) {
			if (coh->GetNumWantedPhysicalStates(objID) <= 1) {
				wps.wantedForwardSpeed = 0.0f;
			} else {
				coh->PopWantedPhysicalStates(objID, 1);
			}

			// FIXME
			coh->SetSimObjectWantedPhysicalState(objID, wps, false);
		}
	}
}

void PathModule::Kill() {
	std::cout << "[PathModule::Kill]" << std::endl;
}



void PathModule::AddObjectToGroup(unsigned int objID, unsigned int groupID) {
	if (objectGroups.find(groupID) == objectGroups.end()) {
		objectGroups[groupID] = std::set<unsigned int>();
	}

	objectGroupIDs[objID] = groupID;
	objectGroups[groupID].insert(objID);
}

bool PathModule::DelObjectFromGroup(unsigned int objID) {
	if (objectGroupIDs.find(objID) != objectGroupIDs.end()) {
		const unsigned int groupID = objectGroupIDs[objID];

		objectGroupIDs.erase(objID);
		objectGroups[groupID].erase(objID);

		if (objectGroups[groupID].empty()) {
			// old group is now empty, delete it
			objectGroups.erase(groupID);
		}

		return true;
	}

	// object was not in a group
	return false;
}
