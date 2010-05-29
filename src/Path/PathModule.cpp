#include <iostream>

#include "./PathModule.hpp"
#include "../Ext/CallOutHandler.hpp"

void PathModule::Init() {
	coh = CallOutHandler::GetInstance();

	std::cout << "[PathModule::Init]" << std::endl;
}

void PathModule::Update() {
	coh->GetMaxSimObjects();
}

void PathModule::Kill() {
	CallOutHandler::FreeInstance(coh);

	std::cout << "[PathModule::Kill]" << std::endl;
}
