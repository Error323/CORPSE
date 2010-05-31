#include <iostream>

#include "./PathModule.hpp"
#include "../Ext/ICallOutHandler.hpp"

void PathModule::OnEvent(const IEvent* event) {
	std::cout << "[PathModule::OnEvent] " << event->str() << std::endl;

	switch (event->GetType()) {
		case EVENT_SIMOBJECT_CREATED: {} break;
		case EVENT_SIMOBJECT_DESTROYED: {} break;
		default: {} break;
	}
}

void PathModule::Init() {
	std::cout << "[PathModule::Init]" << std::endl;
}

void PathModule::Update() {
	coh->GetMaxSimObjects();
}

void PathModule::Kill() {
	std::cout << "[PathModule::Kill]" << std::endl;
}
