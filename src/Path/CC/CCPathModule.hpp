#ifndef PFFG_PATH_MODULE_HDR
#define PFFG_PATH_MODULE_HDR

#include "./CCGrid.hpp"
#include "../IPathModule.hpp"
#include "../../System/IEvent.hpp"

class CCPathModule: public IPathModule {
public:
	CCPathModule(ICallOutHandler* icoh): IPathModule(icoh) {
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

	bool GetScalarDataTypeInfo(DataTypeInfo*) const;
	bool GetVectorDataTypeInfo(DataTypeInfo*) const;
	unsigned int GetNumScalarDataTypes() const { return CCGrid::NUM_SCALAR_DATATYPES; }
	unsigned int GetNumVectorDataTypes() const { return CCGrid::NUM_VECTOR_DATATYPES; }

private:
	typedef std::list<unsigned int> List;
	typedef std::list<unsigned int>::const_iterator ListIt;

	void UpdateGrid(bool);
	void UpdateGroups(bool);
	bool UpdateObjects(const Set&, const Set&);

	void AddObjectToGroup(unsigned int, unsigned int);
	bool DelObjectFromGroup(unsigned int);
	bool DelGroup(unsigned int);

	// each group is updated sequentially, so we only
	// require one grid in which the per-group fields
	// are recycled
	CCGrid mGrid;

	DataTypeInfo cachedScalarData;
	DataTypeInfo cachedVectorData;
};

IPathModule* CALL_CONV GetPathModuleInstance(ICallOutHandler* icoh) { return (new CCPathModule(icoh)); }
void         CALL_CONV FreePathModuleInstance(IPathModule* m) { delete ((CCPathModule*) m); }

#endif
