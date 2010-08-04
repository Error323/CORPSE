#ifndef PFFG_PATH_MODULE_HDR
#define PFFG_PATH_MODULE_HDR

#include <map>
#include <set>

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

	unsigned int GetScalarDataArraySizeX(int) const { return 0; }
	unsigned int GetScalarDataArraySizeZ(int) const { return 0; }
	const float* GetScalarDataArray(int) const { return NULL; }
	unsigned int GetVectorDataArraySizeX(int) const { return 0; }
	unsigned int GetVectorDataArraySizeZ(int) const { return 0; }
	const vec3f* GetVectorDataArray(int) const { return NULL; }

private:
	void AddObjectToGroup(unsigned int, unsigned int);
	bool DelObjectFromGroup(unsigned int);

	unsigned int numGroupIDs;

	std::map<unsigned int, const SimObjectDef*> simObjectIDs;     // object ID ==> object def
	std::map<unsigned int, unsigned int> objectGroupIDs;          // object ID ==> group ID
	std::map<unsigned int, std::set<unsigned int> > objectGroups; // group ID ==> object IDs
};

IPathModule* CALL_CONV GetPathModuleInstance(ICallOutHandler* icoh) { return (new PathModule(icoh)); }
void         CALL_CONV FreePathModuleInstance(IPathModule* m) { delete ((PathModule*) m); }

#endif
