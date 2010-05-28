#include "./SimObjectDefLoader.hpp"
#include "./SimObjectDef.hpp"

std::map<std::string, SimObjectDef*> SimObjectDefLoader::objectDefs;

void SimObjectDefLoader::DelDefs() {
	for (std::map<std::string, SimObjectDef*>::iterator it = objectDefs.begin(); it != objectDefs.end(); it++) {
		delete it->second;
	}
}

SimObjectDef* SimObjectDefLoader::GetDef(const std::string& s) {
	if (objectDefs.find(s) == objectDefs.end()) {
		SimObjectDef* def = new SimObjectDef();
			def->SetModelName(s);
		objectDefs[s] = def;
		return def;
	} else {
		return objectDefs[s];
	}
}
