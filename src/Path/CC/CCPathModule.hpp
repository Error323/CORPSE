#ifndef PFFG_PATH_MODULE_HDR
#define PFFG_PATH_MODULE_HDR

#include <map>
#include <set>

#include "./Grid.hpp"
#include "../IPathModule.hpp"
#include "../../System/IEvent.hpp"

class IEvent;
class SimObjectDef;
class PathModule: public IPathModule {
public:
	PathModule(ICallOutHandler* icoh): IPathModule(icoh) {
		// NOTE:
		//    do not use call-outs here or in destructor,
		//    SimObjectHandler does not exist yet or is
		//    already deleted
		numGroupIDs = 0;
	}

	bool WantsEvent(int eventType) const {
		return
			(eventType == EVENT_SIMOBJECT_CREATED) ||
			(eventType == EVENT_SIMOBJECT_DESTROYED) ||
			(eventType == EVENT_SIMOBJECT_MOVEORDER) ||
			(eventType == EVENT_SIMOBJECT_COLLISION);
	}
	void OnEvent(const IEvent*);

	void Init();
	void Update();
	void Kill();

	enum {
		// scalar fields
		DATATYPE_DENSITY         = 0, // rho (global)
		DATATYPE_DISCOMFORT      = 1, // g (per-group?, stored at cell-centers)
		DATATYPE_POTENTIAL       = 2, // phi (per-group, stored at cell-centers)
		DATATYPE_POTENTIAL_DELTA = 3, // delta-phi (per-group, stored at cell-edges)
		DATATYPE_HEIGHT          = 4, // h (global, stored at cell-centers)
		DATATYPE_HEIGHT_DELTA    = 5, // delta-h (global, stored at cell-edges)

		// vector fields
		DATATYPE_COST            = 6, // C (per-group, stored at cell-edges)
		DATATYPE_SPEED           = 7, // f (per-group, stored at cell-edges)
		DATATYPE_VELOCITY        = 8, // v (per-group, stored at cell-edges)
		DATATYPE_VELOCITY_AVG    = 9, // v-bar (global, stored at cell-centers)
	};

	unsigned int GetScalarDataArraySizeX(unsigned int) const;
	unsigned int GetScalarDataArraySizeZ(unsigned int) const;
	const float* GetScalarDataArray(unsigned int, unsigned int) const;
	unsigned int GetVectorDataArraySizeX(unsigned int) const;
	unsigned int GetVectorDataArraySizeZ(unsigned int) const;
	const vec3f* GetVectorDataArray(unsigned int, unsigned int) const;

private:
	void AddObjectToGroup(unsigned int, unsigned int);
	bool DelObjectFromGroup(unsigned int);

	unsigned int numGroupIDs;

	//! FIXME: we need a grid per group?
	Grid mGrid;

	std::map<unsigned int, const SimObjectDef*> simObjectIDs;     // object ID ==> object def
	std::map<unsigned int, unsigned int> objectGroupIDs;          // object ID ==> group ID
	std::map<unsigned int, std::set<unsigned int> > objectGroups; // group ID ==> object IDs
};

IPathModule* CALL_CONV GetPathModuleInstance(ICallOutHandler* icoh) { return (new PathModule(icoh)); }
void         CALL_CONV FreePathModuleInstance(IPathModule* m) { delete ((PathModule*) m); }

#endif
