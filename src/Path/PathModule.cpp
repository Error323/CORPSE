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
	// steer the objects around the map
	for (std::map<unsigned int, const SimObjectDef*>::const_iterator it = simObjectIDs.begin(); it != simObjectIDs.end(); ++it) {
		const vec3f vec = coh->GetSimObjectWantedPosition(it->first) - coh->GetSimObjectPosition(it->first);

		/*
		const bool b0 = (pos.x > ((coh->GetHeightMapSizeX() * coh->GetSquareSize()) * 0.9f));
		const bool b1 = (pos.x < ((coh->GetHeightMapSizeX() * coh->GetSquareSize()) * 0.1f));
		const bool b2 = (pos.z > ((coh->GetHeightMapSizeZ() * coh->GetSquareSize()) * 0.9f));
		const bool b3 = (pos.z < ((coh->GetHeightMapSizeZ() * coh->GetSquareSize()) * 0.1f));

		if (b0 && b2) { coh->SetSimObjectWantedDirection(it->first, -ZVECf); } // bottom-right
		if (b0 && b3) { coh->SetSimObjectWantedDirection(it->first, -XVECf); } // top-right
		if (b1 && b2) { coh->SetSimObjectWantedDirection(it->first,  XVECf); } // bottom-left
		if (b1 && b3) { coh->SetSimObjectWantedDirection(it->first,  ZVECf); } // top-left
		*/

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
