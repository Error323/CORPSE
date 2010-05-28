#include "./SimObjectHandler.hpp"
#include "./SimObject.hpp"
#include "./SimObjectDef.hpp"
#include "./SimObjectDefLoader.hpp"
#include "../Map/Ground.hpp"
#include "../System/EngineAux.hpp"
#include "../System/LuaParser.hpp"

SimObjectHandler* SimObjectHandler::GetInstance() {
	static SimObjectHandler* soh = NULL;

	if (soh == NULL) {
		soh = new SimObjectHandler();
	}

	return soh;
}

void SimObjectHandler::FreeInstance(SimObjectHandler* soh) {
	delete soh;
}



SimObjectHandler::SimObjectHandler() {
	const LuaTable* rootTable = LUA->GetRoot();
	const LuaTable* objectsTable = rootTable->GetTblVal("objects");

	std::list<int> simObjectIDs;
	objectsTable->GetIntTblKeys(&simObjectIDs);

	for (std::list<int>::iterator it = simObjectIDs.begin(); it != simObjectIDs.end(); it++) {
		const LuaTable* objectTbl = objectsTable->GetTblVal(*it);

		vec3f pos = objectTbl->GetVec<vec3f>("pos", 3);
			pos.y = ground->GetHeight(pos.x, pos.z);
		mat44f mat = mat44f(pos, XVECf, YVECf, ZVECf);
			mat.SetYDirXZ(ground->GetNormal(pos.x, pos.z));

		SimObjectDef* sod = SimObjectDefLoader::GetDef(objectTbl->GetStrVal("mdl", ""));
		SimObject* so = new SimObject(sod);
			so->SetMat(mat);

		simObjects.push_back(so);
	}
}

SimObjectHandler::~SimObjectHandler() {
	for (std::list<SimObject*>::iterator it = simObjects.begin(); it != simObjects.end(); it++) {
		delete *it;
	}
}

void SimObjectHandler::Update() {
	for (std::list<SimObject*>::iterator it = simObjects.begin(); it != simObjects.end(); it++) {
		(*it)->Update();
	}
}
