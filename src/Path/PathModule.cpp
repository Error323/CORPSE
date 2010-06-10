#include <iostream>

#include "./PathModule.hpp"
#include "../Math/vec3.hpp"
#include "../Ext/ICallOutHandler.hpp"
#include "../Sim/SimObjectDef.hpp"

void PathModule::OnEvent(const IEvent* e) {
	std::cout << "[PathModule::OnEvent] " << e->str() << std::endl;

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
		} break;

		case EVENT_SIMOBJECT_MOVEORDER: {
			const SimObjectMoveOrderEvent* ee = dynamic_cast<const SimObjectMoveOrderEvent*>(e);

			const std::list<unsigned int>& objectIDs = ee->GetObjectIDs();
			const vec3f& goalPos = ee->GetGoalPos();

			// TODO: group management
			for (std::list<unsigned int>::const_iterator it = objectIDs.begin(); it != objectIDs.end(); ++it) {
				assert(coh->IsValidSimObjectID(*it));

				coh->SetSimObjectWantedPosition(*it, goalPos);
				coh->SetSimObjectWantedDirection(*it, (goalPos - coh->GetSimObjectPosition(*it)).norm());
				coh->SetSimObjectWantedForwardSpeed(*it, simObjectIDs[*it]->GetMaxForwardSpeed());
			}
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
		const vec3f vec = coh->GetSimObjectWantedPosition(it->first) - coh->GetSimObjectPosition(it->first);

		if (vec.sqLen3D() > (coh->GetSquareSize() * coh->GetSquareSize())) {
			coh->SetSimObjectWantedDirection(it->first, vec.norm());
		} else {
			coh->SetSimObjectWantedForwardSpeed(it->first, 0.0f);
		}
	}
}

void PathModule::Kill() {
	std::cout << "[PathModule::Kill]" << std::endl;
}
