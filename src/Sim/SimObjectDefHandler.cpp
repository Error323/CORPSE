#include "./SimObjectDefHandler.hpp"
#include "./SimObjectDef.hpp"
#include "../System/EngineAux.hpp"
#include "../System/LuaParser.hpp"
#include "../System/Server.hpp"

SimObjectDefHandler* SimObjectDefHandler::GetInstance() {
	static SimObjectDefHandler* sodh = NULL;
	static unsigned int depth = 0;

	if (sodh == NULL) {
		assert(depth == 0);

		depth += 1;
		sodh = new SimObjectDefHandler();
		depth -= 1;
	}

	return sodh;
}

void SimObjectDefHandler::FreeInstance(SimObjectDefHandler* sodh) {
	delete sodh;
}



bool SimObjectDefHandler::LoadDefs() {
	const LuaTable* rootTable = LUA->GetRoot();
	const LuaTable* objectDefsTable = rootTable->GetTblVal("objectdefs");

	std::list<std::string> simObjectDefKeys;
	objectDefsTable->GetStrTblKeys(&simObjectDefKeys);

	if (simObjectDefKeys.empty()) {
		return false;
	}

	objectDefsVec.resize(simObjectDefKeys.size(), NULL);

	for (std::list<std::string>::iterator it = simObjectDefKeys.begin(); it != simObjectDefKeys.end(); it++) {
		const LuaTable* objectDefTable = objectDefsTable->GetTblVal(*it);

		// convert to per-frame units
		SimObjectDef* def = new SimObjectDef(objectDefsMap.size());
			def->SetModelName(objectDefTable->GetStrVal("mdl", ""));
			def->SetMaxForwardSpeed(objectDefTable->GetFltVal("maxForwardSpeed", 0.0f) / server->GetSimFrameRate());
			def->SetMaxTurningRate(objectDefTable->GetFltVal("maxTurningRate", 0.0f) / server->GetSimFrameRate());
			def->SetMaxAccelerationRate(objectDefTable->GetFltVal("maxAccelerationRate", 0.0f) / server->GetSimFrameRate());
			def->SetMaxDeccelerationRate(objectDefTable->GetFltVal("maxDeccelerationRate", 0.0f) / server->GetSimFrameRate());
			def->SetMaxSlopeAngleCosine(objectDefTable->GetFltVal("maxSlopeAngleCosine", 1.0f));

		objectDefsMap[*it] = def;
		objectDefsVec[def->GetID()] = def;
	}

	assert(objectDefsMap.size() == objectDefsVec.size());
	return true;
}

void SimObjectDefHandler::DelDefs() {
	for (std::map<std::string, SimObjectDef*>::iterator it = objectDefsMap.begin(); it != objectDefsMap.end(); it++) {
		delete it->second;
	}
}

SimObjectDef* SimObjectDefHandler::GetDef(const std::string& defName) {
	std::map<std::string, SimObjectDef*>::const_iterator it = objectDefsMap.find(defName);

	if (it == objectDefsMap.end()) {
		return NULL;
	} else {
		return it->second;
	}
}

SimObjectDef* SimObjectDefHandler::GetDef(unsigned int defID) {
	if (defID < objectDefsVec.size()) {
		return objectDefsVec[defID];
	}

	return NULL;
}
