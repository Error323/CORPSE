#include "./SimObjectDefLoader.hpp"
#include "./SimObjectDef.hpp"
#include "../System/EngineAux.hpp"
#include "../System/LuaParser.hpp"
#include "../System/Server.hpp"

std::map<std::string, SimObjectDef*> SimObjectDefLoader::objectDefs;

bool SimObjectDefLoader::LoadDefs() {
	const LuaTable* rootTable = LUA->GetRoot();
	const LuaTable* objectDefsTable = rootTable->GetTblVal("objectdefs");

	std::list<std::string> simObjectDefKeys;
	objectDefsTable->GetStrTblKeys(&simObjectDefKeys);

	if (simObjectDefKeys.empty()) {
		return false;
	}

	for (std::list<std::string>::iterator it = simObjectDefKeys.begin(); it != simObjectDefKeys.end(); it++) {
		const LuaTable* objectDefTable = objectDefsTable->GetTblVal(*it);

		// convert to per-frame units
		SimObjectDef* def = new SimObjectDef();
			def->SetModelName(objectDefTable->GetStrVal("mdl", ""));
			def->SetMaxForwardSpeed(objectDefTable->GetFltVal("maxForwardSpeed", 0.0f) / server->GetSimFrameRate());
			def->SetMaxTurningRate(objectDefTable->GetFltVal("maxTurningRate", 0.0f) / server->GetSimFrameRate());
			def->SetMaxAccelerationRate(objectDefTable->GetFltVal("maxAccelerationRate", 0.0f) / server->GetSimFrameRate());
			def->SetMaxDeccelerationRate(objectDefTable->GetFltVal("maxDeccelerationRate", 0.0f) / server->GetSimFrameRate());

		objectDefs[*it] = def;
	}

	return true;
}

void SimObjectDefLoader::DelDefs() {
	for (std::map<std::string, SimObjectDef*>::iterator it = objectDefs.begin(); it != objectDefs.end(); it++) {
		delete it->second;
	}
}

SimObjectDef* SimObjectDefLoader::GetDef(const std::string& defName) {
	std::map<std::string, SimObjectDef*>::const_iterator it = objectDefs.find(defName);

	if (it == objectDefs.end()) {
		return NULL;
	} else {
		return it->second;
	}
}
