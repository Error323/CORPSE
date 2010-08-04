#include <iostream>

#include "./CCPathModule.hpp"
#include "../../Math/vec3.hpp"
#include "../../Ext/ICallOutHandler.hpp"
#include "../../Sim/SimObjectDef.hpp"
#include "../../Sim/SimObjectState.hpp"

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
				PFFG_ASSERT(objectGroupIDs.empty());
				PFFG_ASSERT(objectGroups.empty());

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
				const vec3f& objPos = coh->GetSimObjectPosition(objID);

				PFFG_ASSERT(coh->IsValidSimObjectID(objID));

				DelObjectFromGroup(objID);
				AddObjectToGroup(objID, groupID);
			}
		} break;

		case EVENT_SIMOBJECT_COLLISION: {

		} break;

		default: {
			PFFG_ASSERT_MSG(false, "PathModule::OnEvent switch case (%d) doesn't exist", e->GetType());
		} break;
	}
}

void PathModule::Init() {
	std::cout << "[CCPathModule::Init]" << std::endl;
	mGrid.Init(2, coh);
}

void PathModule::Update() {
	// Reset the touched cells in the grid
	mGrid.Reset();

	// Convert the crowd into a density field
	std::map<unsigned int, const SimObjectDef*>::iterator i;
	for (i = simObjectIDs.begin(); i != simObjectIDs.end(); i++) {
		const vec3f& objPos = coh->GetSimObjectPosition(i->first);
		vec3f objVel = coh->GetSimObjectDirection(i->first);
		objVel *= coh->GetSimObjectCurrentForwardSpeed(i->first);
		mGrid.AddDensityAndSpeed(objPos, objVel);
	}

	// Foreach group
	std::map<unsigned int, std::set<unsigned int> >::iterator j;
	for (j = objectGroups.begin(); j != objectGroups.end(); j++) {
		// Construct the unit cost field
		// Construct the potential and the gradient
		// Update the object locations
	}

	// Enforce minimum distance between objects
}

void PathModule::Kill() {
	std::cout << "[CCPathModule::Kill]" << std::endl;
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
