#include <iostream>

#include "./PathModule.hpp"
#include "../Math/vec3.hpp"
#include "../Ext/ICallOutHandler.hpp"

void PathModule::OnEvent(const IEvent* e) {
	std::cout << "[PathModule::OnEvent] " << e->str() << std::endl;

	switch (e->GetType()) {
		case EVENT_SIMOBJECT_CREATED: {
			const SimObjectCreatedEvent* ee = dynamic_cast<const SimObjectCreatedEvent*>(e);
			const unsigned int objectID = ee->GetObjectID();

			simObjectIDs.insert(objectID);
		} break;
		case EVENT_SIMOBJECT_DESTROYED: {
			const SimObjectDestroyedEvent* ee = dynamic_cast<const SimObjectDestroyedEvent*>(e);
			const unsigned int objectID = ee->GetObjectID();

			simObjectIDs.erase(objectID);
		} break;

		default: {
		} break;
	}
}

void PathModule::Init() {
	std::cout << "[PathModule::Init]" << std::endl;
}

void PathModule::Update() {
	for (std::set<unsigned int>::const_iterator it = simObjectIDs.begin(); it != simObjectIDs.end(); ++it) {
		const vec3f& pos = coh->GetSimObjectPosition(*it);
		const vec3f& dir = coh->GetSimObjectDirection(*it);

		const bool b0 = (pos.x > ((coh->GetHeightMapSizeX() * coh->GetSquareSize()) * 0.9f));
		const bool b1 = (pos.x < ((coh->GetHeightMapSizeX() * coh->GetSquareSize()) * 0.1f));
		const bool b2 = (pos.z > ((coh->GetHeightMapSizeZ() * coh->GetSquareSize()) * 0.9f));
		const bool b3 = (pos.z < ((coh->GetHeightMapSizeZ() * coh->GetSquareSize()) * 0.1f));

		if (b0 && b2) { coh->SetSimObjectWantedDirection(*it, -ZVECf); } // bottom-right
		if (b0 && b3) { coh->SetSimObjectWantedDirection(*it, -XVECf); } // top-right
		if (b1 && b2) { coh->SetSimObjectWantedDirection(*it,  XVECf); } // bottom-left
		if (b1 && b3) { coh->SetSimObjectWantedDirection(*it,  ZVECf); } // top-left
	}
}

void PathModule::Kill() {
	std::cout << "[PathModule::Kill]" << std::endl;
}
