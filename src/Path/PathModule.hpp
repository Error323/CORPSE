#ifndef PFFG_PATH_MODULE_HDR
#define PFFG_PATH_MODULE_HDR

#include <map>

#include "./IPathModule.hpp"
#include "../System/IEvent.hpp"

class IEvent;
class SimObjectDef;
class PathModule: public IPathModule {
public:
	PathModule(ICallOutHandler* icoh): IPathModule(icoh) {
		// NOTE:
		//    do not use call-outs here or in destructor,
		//    SimObjectHandler does not exist yet or is
		//    already deleted
	}

	bool WantsEvent(int eventType) const {
		return
			(eventType == EVENT_SIMOBJECT_CREATED) ||
			(eventType == EVENT_SIMOBJECT_DESTROYED) ||
			(eventType == EVENT_SIMOBJECT_MOVEORDER);
	}
	void OnEvent(const IEvent*);

	void Init();
	void Update();
	void Kill();

private:
	std::map<unsigned int, const SimObjectDef*> simObjectIDs;
};

IPathModule* CALL_CONV GetPathModuleInstance(ICallOutHandler* icoh) { return (new PathModule(icoh)); }
void         CALL_CONV FreePathModuleInstance(IPathModule* m) { delete ((PathModule*) m); }

#endif
