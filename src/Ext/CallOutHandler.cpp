#include "./CallOutHandler.hpp"
#include "../Map/ReadMap.hpp"
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



int CallOutHandler::GetHeightMapSizeX() const { return readMap->mapx; }
int CallOutHandler::GetHeightMapSizeZ() const { return readMap->mapy; }
float CallOutHandler::GetMinMapHeight() const { return readMap->minheight; }
float CallOutHandler::GetMaxMapHeight() const { return readMap->maxheight; }
const float* CallOutHandler::GetCenterHeightMap() const { return &readMap->centerheightmap[0]; }
const float* CallOutHandler::GetCornerHeightMap() const { return readMap->GetHeightmap(); }
const float* CallOutHandler::GetSlopeMap() const { return &readMap->slopemap[0]; }



unsigned int CallOutHandler::GetMaxSimObjects() const {
	return simObjectHandler->GetMaxSimObjects();
}

unsigned int CallOutHandler::GetFreeSimObjectIDs(unsigned int* array, unsigned int size) const {
	const std::set<unsigned int>& freeIDs = simObjectHandler->GetSimObjectFreeIDs();

	unsigned int n = 0;

	for (std::set<unsigned int>::const_iterator it = freeIDs.begin(); it != freeIDs.end() && n < size; ++it) {
		array[n++] = *it;
	}

	return n;
}

unsigned int CallOutHandler::GetUsedSimObjectIDs(unsigned int* array, unsigned int size) const {
	const std::set<unsigned int>& usedIDs = simObjectHandler->GetSimObjectUsedIDs();

	unsigned int n = 0;

	for (std::set<unsigned int>::const_iterator it = usedIDs.begin(); it != usedIDs.end() && n < size; ++it) {
		array[n++] = *it;
	}

	return n;
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



void CallOutHandler::SetObjectMatrix(unsigned int id, const mat44f& m) const {
	if (id < simObjectHandler->GetMaxSimObjects()) {
		if (simObjectHandler->GetSimObject(id) != NULL) {
			SimObject* o = simObjectHandler->GetSimObject(id);
			o->SetMat(m);
		}
	}
}

void CallOutHandler::SetObjectPosition(unsigned int id, const vec3f& v) const {
	if (id < simObjectHandler->GetMaxSimObjects()) {
		if (simObjectHandler->GetSimObject(id) != NULL) {
			SimObject* o = simObjectHandler->GetSimObject(id);
			mat44f m = o->GetMat();

			m.SetPos(v);
			o->SetMat(m);
		}
	}
}

void CallOutHandler::SetObjectDirection(unsigned int id, const vec3f& v) const {
	if (id < simObjectHandler->GetMaxSimObjects()) {
		if (simObjectHandler->GetSimObject(id) != NULL) {
			SimObject* o = simObjectHandler->GetSimObject(id);
			mat44f m = o->GetMat();

			m.SetZDir(v);
			m.SetXDir((v.cross(m.GetYDir())).norm());
			m.SetYDir((v.cross(m.GetXDir())).norm());
			o->SetMat(m);
		}
	}
}
