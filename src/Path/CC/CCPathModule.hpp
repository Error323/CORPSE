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
		DATATYPE_DENSITY         =  0, // rho (global)
		DATATYPE_DISCOMFORT      =  1, // g (per-group?, stored at cell-centers)
		DATATYPE_SPEED           =  2, // f (per-group, stored at cell-edges)
		DATATYPE_COST            =  5, // C (per-group, stored at cell-edges)
		DATATYPE_POTENTIAL       =  3, // phi (per-group, stored at cell-centers)
		DATATYPE_HEIGHT          =  4, // h (global, stored at cell-centers)

		// vector fields
		DATATYPE_VELOCITY        =  6, // v (per-group, stored at cell-edges)
		DATATYPE_VELOCITY_AVG    =  7, // v-bar (global, stored at cell-centers)
		DATATYPE_POTENTIAL_DELTA =  8, // delta-phi (per-group, stored at cell-edges)
		DATATYPE_HEIGHT_DELTA    =  9, // delta-h (global, stored at cell-edges)

		NUM_DATATYPES            = 10,
	};

	unsigned int GetNumGroupIDs() const { return mObjectGroups.size(); }
	unsigned int GetGroupIDs(unsigned int* array, unsigned int size) const {
		unsigned int n = 0;

		std::map<unsigned int, std::set<unsigned int> >::const_iterator it;
		for (it = mObjectGroups.begin(); it != mObjectGroups.end() && n < size; ++it) {
			array[n++] = it->first;
		}

		return n;
	}

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

	//! each group is updated sequentially, so we only
	//! require one grid in which the per-group fields
	//! are recycled
	Grid mGrid;

	std::map<unsigned int, std::vector<Grid::Cell*> > mGoals;
	std::map<unsigned int, const SimObjectDef*> mSimObjectIDs;     // object ID ==> object def
	std::map<unsigned int, unsigned int> mObjectGroupIDs;          // object ID ==> group ID
	std::map<unsigned int, std::set<unsigned int> > mObjectGroups; // group ID ==> object IDs
};

IPathModule* CALL_CONV GetPathModuleInstance(ICallOutHandler* icoh) { return (new PathModule(icoh)); }
void         CALL_CONV FreePathModuleInstance(IPathModule* m) { delete ((PathModule*) m); }

#endif
