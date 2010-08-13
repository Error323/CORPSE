#ifndef PFFG_PATH_MODULE_HDR
#define PFFG_PATH_MODULE_HDR

#include <map>
#include <set>

#include "./Grid.hpp"
#include "../IPathModule.hpp"
#include "../../System/IEvent.hpp"

class IEvent;
class SimObjectDef;
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

	unsigned int GetNumGroupIDs() const { return mGroups.size(); }
	unsigned int GetGroupIDs(unsigned int* array, unsigned int size) const {
		unsigned int n = 0;

		std::map<unsigned int, MGroup*>::const_iterator it;
		for (it = mGroups.begin(); it != mGroups.end() && n < size; ++it) {
			array[n++] = it->first;
		}

		return n;
	}

	bool IsGlobalDataType(unsigned int dataType) const {
		return
			(dataType == Grid::DATATYPE_DENSITY || dataType == Grid::DATATYPE_HEIGHT ||
			 dataType == Grid::DATATYPE_HEIGHT_DELTA  || dataType == Grid::DATATYPE_VELOCITY_AVG);
	}
	unsigned int GetNumScalarDataTypes() const { return Grid::NUM_SCALAR_DATATYPES; }
	unsigned int GetScalarDataArraySizeX(unsigned int) const;
	unsigned int GetScalarDataArraySizeZ(unsigned int) const;
	unsigned int GetScalarDataArrayStride(unsigned int) const;
	const float* GetScalarDataArray(unsigned int, unsigned int) const;
	unsigned int GetNumVectorDataTypes() const { return Grid::NUM_VECTOR_DATATYPES; }
	unsigned int GetVectorDataArraySizeX(unsigned int) const;
	unsigned int GetVectorDataArraySizeZ(unsigned int) const;
	unsigned int GetVectorDataArrayStride(unsigned int) const;
	const vec3f* GetVectorDataArray(unsigned int, unsigned int) const;

private:
	void AddObjectToGroup(unsigned int, unsigned int);
	bool DelObjectFromGroup(unsigned int);
	bool DelGroup(unsigned int);

	unsigned int numGroupIDs;

	// each group is updated sequentially, so we only
	// require one grid in which the per-group fields
	// are recycled
	Grid mGrid;

	struct MObject {
		public:
			MObject(): mGroupID(-1), mObjectDef(NULL) {}
			MObject(const SimObjectDef* def): mGroupID(-1), mObjectDef(def) {}

			void SetGroupID(unsigned int gID) { mGroupID = gID; }
			unsigned int GetGroupID() const { return mGroupID; }

		private:
			unsigned int mGroupID;
			const SimObjectDef* mObjectDef;
	};

	struct MGroup {
		public:
			MGroup(): mGoalID(-1) {}
			~MGroup() { mObjectIDs.clear(); }

			void DelObject(unsigned int objectID) { mObjectIDs.erase(objectID); }
			void AddObject(unsigned int objectID) { mObjectIDs.insert(objectID); }
			unsigned int GetSize() const { return mObjectIDs.size(); }
			bool IsEmpty() const { return mObjectIDs.empty(); }
			const std::set<unsigned int>& GetObjectIDs() const { return mObjectIDs; }

			void SetGoal(const unsigned int goalID) { mGoalID = goalID; }
			unsigned int GetGoal() const { return mGoalID; }

		private:
			unsigned int mGoalID;
			std::set<unsigned int> mObjectIDs;  // unit member ID's
	};

	std::map<unsigned int, MGroup*> mGroups;
	std::map<unsigned int, MObject*> mObjects;
};

IPathModule* CALL_CONV GetPathModuleInstance(ICallOutHandler* icoh) { return (new CCPathModule(icoh)); }
void         CALL_CONV FreePathModuleInstance(IPathModule* m) { delete ((CCPathModule*) m); }

#endif
