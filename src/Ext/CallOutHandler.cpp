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



unsigned int CallOutHandler::GetMaxSimObjects() const { return simObjectHandler->GetMaxSimObjects(); }
unsigned int CallOutHandler::GetNumSimObjects() const { return simObjectHandler->GetNumSimObjects(); }

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

bool CallOutHandler::IsValidSimObjectID(unsigned int id) const {
	return ((id < simObjectHandler->GetMaxSimObjects()) && (simObjectHandler->GetSimObject(id) != NULL));
}



const mat44f& CallOutHandler::GetSimObjectMatrix(unsigned int id) const {
	if (IsValidSimObjectID(id)) {
		const SimObject* o = simObjectHandler->GetSimObject(id);
		const mat44f& m = o->GetMat();

		return m;
	}

	static const mat44f m;
	return m;
}

const vec3f& CallOutHandler::GetSimObjectPosition(unsigned int id) const {
	if (IsValidSimObjectID(id)) {
		const SimObject* o = simObjectHandler->GetSimObject(id);
		const mat44f& m = o->GetMat();

		return (m.GetPos());
	}

	return NVECf;
}

const vec3f& CallOutHandler::GetSimObjectDirection(unsigned int id) const {
	if (IsValidSimObjectID(id)) {
		const SimObject* o = simObjectHandler->GetSimObject(id);
		const mat44f& m = o->GetMat();

		return (m.GetZDir());
	}

	return NVECf;
}



void CallOutHandler::SetSimObjectWantedForwardSpeed(unsigned int id, float spd) const {
	if (IsValidSimObjectID(id)) {
		simObjectHandler->GetSimObject(id)->SetWantedForwardSpeed(spd);
	}
}

void CallOutHandler::SetSimObjectWantedPosition(unsigned int id, const vec3f& pos) const {
	if (IsValidSimObjectID(id)) {
		simObjectHandler->GetSimObject(id)->SetWantedPosition(pos);
	}
}

void CallOutHandler::SetSimObjectWantedDirection(unsigned int id, const vec3f& dir) const {
	if (IsValidSimObjectID(id)) {
		simObjectHandler->GetSimObject(id)->SetWantedDirection(dir);
	}
}
