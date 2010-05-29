#include "./CallOutModule.hpp"
#include "../Math/mat44.hpp"
#include "../Math/vec3.hpp"
#include "../Sim/SimObject.hpp"
#include "../Sim/SimObjectHandler.hpp"

const mat44f& CallOutModule::GetObjectMatrix(unsigned int id) const {
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

const vec3f& CallOutModule::GetObjectPosition(unsigned int id) const {
	if (id < simObjectHandler->GetMaxSimObjects()) {
		if (simObjectHandler->GetSimObject(id) != NULL) {
			const SimObject* o = simObjectHandler->GetSimObject(id);
			const mat44f& m = o->GetMat();

			return (m.GetPos());
		}
	}

	return NVECf;
}

const vec3f& CallOutModule::GetObjectDirection(unsigned int id) const {
	if (id < simObjectHandler->GetMaxSimObjects()) {
		if (simObjectHandler->GetSimObject(id) != NULL) {
			const SimObject* o = simObjectHandler->GetSimObject(id);
			const mat44f& m = o->GetMat();

			return (m.GetZDir());
		}
	}

	return NVECf;
}
