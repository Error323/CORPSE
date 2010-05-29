#include "./CallOutHandler.hpp"
#include "../Math/mat44.hpp"
#include "../Math/vec3.hpp"
#include "../Sim/SimObject.hpp"
#include "../Sim/SimObjectHandler.hpp"

CallOutHandler* CallOutHandler::GetInstance() {
	static CallOutHandler* coh = NULL;

	if (coh == NULL) {
		coh = new CallOutHandler();
	}

	return coh;
}

void CallOutHandler::FreeInstance(CallOutHandler* coh) {
	delete coh;
}



unsigned int CallOutHandler::GetMaxSimObjects() const {
	return simObjectHandler->GetMaxSimObjects();
}

const mat44f& CallOutHandler::GetObjectMatrix(unsigned int id) const {
	if (id < simObjectHandler->GetMaxSimObjects()) {
		if (simObjectHandler->GetSimObject(id) != NULL) {
			const SimObject* o = simObjectHandler->GetSimObject(id);
			const mat44f& m = o->GetMat();

			return m;
		}
	}

	static mat44f m;
	return m;
}

const vec3f& CallOutHandler::GetObjectPosition(unsigned int id) const {
	if (id < simObjectHandler->GetMaxSimObjects()) {
		if (simObjectHandler->GetSimObject(id) != NULL) {
			const SimObject* o = simObjectHandler->GetSimObject(id);
			const mat44f& m = o->GetMat();

			return (m.GetPos());
		}
	}

	return NVECf;
}

const vec3f& CallOutHandler::GetObjectDirection(unsigned int id) const {
	if (id < simObjectHandler->GetMaxSimObjects()) {
		if (simObjectHandler->GetSimObject(id) != NULL) {
			const SimObject* o = simObjectHandler->GetSimObject(id);
			const mat44f& m = o->GetMat();

			return (m.GetZDir());
		}
	}

	return NVECf;
}
