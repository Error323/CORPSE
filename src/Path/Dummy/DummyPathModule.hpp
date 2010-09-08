#ifndef PFFG_PATH_MODULE_HDR
#define PFFG_PATH_MODULE_HDR

#include "../IPathModule.hpp"
#include "../../System/IEvent.hpp"

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

	bool GetScalarDataTypeInfo(DataTypeInfo*) const { return false; }
	bool GetVectorDataTypeInfo(DataTypeInfo*) const { return false; }
	unsigned int GetNumScalarDataTypes() const { return 0; }
	unsigned int GetNumVectorDataTypes() const { return 0; }

private:
	void AddObjectToGroup(unsigned int, unsigned int);
	bool DelObjectFromGroup(unsigned int);
	bool DelGroup(unsigned int);
};

IPathModule* CALL_CONV GetPathModuleInstance(ICallOutHandler* icoh) { return (new DummyPathModule(icoh)); }
void         CALL_CONV FreePathModuleInstance(IPathModule* m) { delete ((DummyPathModule*) m); }

#endif
