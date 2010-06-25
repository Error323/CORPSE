#include "./CallOutHandler.hpp"
#include "../Map/ReadMap.hpp"
#include "../Math/mat44.hpp"
#include "../Math/vec3.hpp"
#include "../Sim/SimObject.hpp"
#include "../Sim/SimObjectDef.hpp"
#include "../Sim/SimObjectHandler.hpp"
#include "../Sim/SimObjectDefHandler.hpp"
#include "../Sim/SimObjectGrid.hpp"

CallOutHandler* CallOutHandler::GetInstance() {
	static CallOutHandler* coh = NULL;
	static unsigned int depth = 0;

	if (coh == NULL) {
		assert(depth == 0);

		depth += 1;
		coh = new CallOutHandler();
		depth -= 1;
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
int CallOutHandler::GetSquareSize() const { return readMap->SQUARE_SIZE; }
const float* CallOutHandler::GetCenterHeightMap() const { return &readMap->centerheightmap[0]; }
const float* CallOutHandler::GetCornerHeightMap() const { return readMap->GetHeightmap(); }
const float* CallOutHandler::GetSlopeMap() const { return &readMap->slopemap[0]; }



unsigned int CallOutHandler::GetNumSimObjectDefs() const { return simObjectDefHandler->GetNumDefs(); }
const SimObjectDef* CallOutHandler::GetRawSimObjectDef(unsigned int defID) const { return simObjectDefHandler->GetDef(defID); }

unsigned int CallOutHandler::GetMaxSimObjects() const { return simObjectHandler->GetMaxSimObjects(); }
unsigned int CallOutHandler::GetNumSimObjects() const { return simObjectHandler->GetNumSimObjects(); }


unsigned int CallOutHandler::GetObjectIDs(const vec3f& pos, const vec3f& radii, unsigned int* array, unsigned int size) const {
	SimObjectGrid<const SimObject*>* grid = simObjectHandler->GetSimObjectGrid();

	std::list<const SimObject*> objects;
	grid->GetObjects(pos, radii, objects);

	unsigned int n = 0;

	// copy the object ID's
	for (std::list<const SimObject*>::const_iterator it = objects.begin(); it != objects.end() && n < size; ++it) {
		array[n++] = (*it)->GetID();
	}

	return n;
}

unsigned int CallOutHandler::GetClosestObjectID(const vec3f& pos, float radius) const {
	const SimObject* o = simObjectHandler->GetClosestSimObject(pos, radius);

	if (o == NULL) {
		return -1;
	} else {
		return o->GetID();
	}
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

bool CallOutHandler::IsValidSimObjectID(unsigned int id) const {
	return (simObjectHandler->IsValidSimObjectID(id));
}



const SimObjectDef* CallOutHandler::GetSimObjectDef(unsigned int id) const {
	if (IsValidSimObjectID(id)) {
		const SimObject* o = simObjectHandler->GetSimObject(id);
		const SimObjectDef* d = o->GetDef();

		return d;
	}

	return NULL;
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

float CallOutHandler::GetSimObjectCurrentForwardSpeed(unsigned int id) const {
	if (IsValidSimObjectID(id)) {
		const SimObject* o = simObjectHandler->GetSimObject(id);
		const PhysicalState& ps = o->GetPhysicalState();

		return ps.currentForwardSpeed;
	}

	return 0.0f;
}



unsigned int CallOutHandler::GetSimObjectNumWantedPhysicalStates(unsigned int id) const {
	if (IsValidSimObjectID(id)) {
		const SimObject* so = simObjectHandler->GetSimObject(id);
		const std::list<WantedPhysicalState>& wpsl = so->GetWantedPhysicalStates();
		return wpsl.size();
	}

	return 0;
}

void CallOutHandler::PushSimObjectWantedPhysicalState(unsigned int objID, const WantedPhysicalState& state, bool queued, bool front) const {
	if (IsValidSimObjectID(objID)) {
		SimObject* so = simObjectHandler->GetSimObject(objID);
		so->PushWantedPhysicalState(state, queued, front);
	}
}

bool CallOutHandler::PopSimObjectWantedPhysicalStates(unsigned int id, unsigned int numStates, bool front) const {
	if (IsValidSimObjectID(id)) {
		SimObject* so = simObjectHandler->GetSimObject(id);
		bool ret = so->PopWantedPhysicalStates(numStates, front);
		return ret;
	}

	return false;
}



const WantedPhysicalState& CallOutHandler::GetSimObjectWantedPhysicalState(unsigned int objID, bool front) const {
	static WantedPhysicalState swps;

	if (IsValidSimObjectID(objID)) {
		const SimObject* so = simObjectHandler->GetSimObject(objID);
		const WantedPhysicalState& wps = so->GetWantedPhysicalState(front);
		return wps;
	}

	return swps;
}
