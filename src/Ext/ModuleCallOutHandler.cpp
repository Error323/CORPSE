#include "./ModuleCallOutHandler.hpp"
#include "../Math/mat44.hpp"
#include "../Math/vec3.hpp"
#include "../Sim/SimObject.hpp"
#include "../Sim/SimObjectHandler.hpp"

ModuleCallOutHandler* ModuleCallOutHandler::GetInstance() {
	static ModuleCallOutHandler* mcoh = NULL;

	if (mcoh == NULL) {
		mcoh = new ModuleCallOutHandler();
	}

	return mcoh;
}

void ModuleCallOutHandler::FreeInstance(ModuleCallOutHandler* mcoh) {
	delete mcoh;
}



const vec3f& ModuleCallOutHandler::GetObjectPosition(unsigned int id) const {
	if (id < simObjectHandler->GetMaxSimObjects()) {
		if (simObjectHandler->GetSimObject(id) != NULL) {
			const SimObject* o = simObjectHandler->GetSimObject(id);
			const mat44f& m = o->GetMat();

			return (m.GetPos());
		}
	}

	return NVECf;
}

const vec3f& ModuleCallOutHandler::GetObjectDirection(unsigned int id) const {
	if (id < simObjectHandler->GetMaxSimObjects()) {
		if (simObjectHandler->GetSimObject(id) != NULL) {
			const SimObject* o = simObjectHandler->GetSimObject(id);
			const mat44f& m = o->GetMat();

			return (m.GetZDir());
		}
	}

	return NVECf;
}
