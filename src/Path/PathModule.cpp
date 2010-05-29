#include <iostream>

#include "./PathModule.hpp"
#include "../Ext/ICallOutHandler.hpp"

void PathModule::Init() {
	std::cout << "[PathModule::Init]" << std::endl;
}

void PathModule::Update() {
	coh->GetMaxSimObjects();
}

void PathModule::Kill() {
	std::cout << "[PathModule::Kill]" << std::endl;
}
