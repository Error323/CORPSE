#ifndef PFFG_PATH_MODULE_HDR
#define PFFG_PATH_MODULE_HDR

#include <map>
#include <set>

#include "../IPathModule.hpp"
#include "../../System/IEvent.hpp"

class IEvent;
class SimObjectDef;
class DummyPathModule: public IPathModule {
public:
	DummyPathModule(ICallOutHandler* icoh): IPathModule(icoh) {
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

	unsigned int GetNumGroupIDs() const { return objectGroups.size(); }
	unsigned int GetGroupIDs(unsigned int* array, unsigned int size) const {
		unsigned int n = 0;

		std::map<unsigned int, std::set<unsigned int> >::const_iterator it;
		for (it = objectGroups.begin(); it != objectGroups.end() && n < size; ++it) {
			array[n++] = it->first;
		}

		return n;
	}

	bool IsGlobalDataType(unsigned int) const { return false; }
	unsigned int GetNumScalarDataTypes() const { return 0; }
	unsigned int GetScalarDataArraySizeX(unsigned int) const { return 0; }
	unsigned int GetScalarDataArraySizeZ(unsigned int) const { return 0; }
	unsigned int GetScalarDataArrayStride(unsigned int) const { return 0; }
	const float* GetScalarDataArray(unsigned int, unsigned int) const { return NULL; }
	unsigned int GetNumVectorDataTypes() const { return 0; }
	unsigned int GetVectorDataArraySizeX(unsigned int) const { return 0; }
	unsigned int GetVectorDataArraySizeZ(unsigned int) const { return 0; }
	unsigned int GetVectorDataArrayStride(unsigned int) const { return 0; }
	const vec3f* GetVectorDataArray(unsigned int, unsigned int) const { return NULL; }

private:
	void AddObjectToGroup(unsigned int, unsigned int);
	bool DelObjectFromGroup(unsigned int);

	// running counter used to assign ID's to new groups
	// (not the actual number of currently active groups)
	unsigned int numGroupIDs;

	std::map<unsigned int, const SimObjectDef*> simObjectIDs;     // object ID ==> object def
	std::map<unsigned int, unsigned int> objectGroupIDs;          // object ID ==> group ID
	std::map<unsigned int, std::set<unsigned int> > objectGroups; // group ID ==> object IDs
};

IPathModule* CALL_CONV GetPathModuleInstance(ICallOutHandler* icoh) { return (new DummyPathModule(icoh)); }
void         CALL_CONV FreePathModuleInstance(IPathModule* m) { delete ((DummyPathModule*) m); }

#endif
