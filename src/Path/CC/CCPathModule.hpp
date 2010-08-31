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

	bool GetScalarDataTypeInfo(DataTypeInfo*) const;
	bool GetVectorDataTypeInfo(DataTypeInfo*) const;
	unsigned int GetNumScalarDataTypes() const { return Grid::NUM_SCALAR_DATATYPES; }
	unsigned int GetNumVectorDataTypes() const { return Grid::NUM_VECTOR_DATATYPES; }

private:
	void UpdateGrid();
	void UpdateGroups();

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
			MObject(): mGroupID(-1), mObjectDef(NULL), mArrived(false) {}
			MObject(const SimObjectDef* def): mGroupID(-1), mObjectDef(def) {}

			const SimObjectDef* GetDef() const { return mObjectDef; }
			void SetGroupID(unsigned int gID) { mGroupID = gID; }
			unsigned int GetGroupID() const { return mGroupID; }

			void SetArrived(bool b) { mArrived = b; }
			bool HasArrived() const { return mArrived; }

		private:
			unsigned int mGroupID;
			const SimObjectDef* mObjectDef;
			bool mArrived;
	};

	struct MGroup {
		public:
			MGroup() {}
			~MGroup() { mObjectIDs.clear(); mGoalIDs.clear(); }

			void DelObject(unsigned int objectID) { mObjectIDs.erase(objectID); }
			void AddObject(unsigned int objectID) { mObjectIDs.insert(objectID); }
			unsigned int GetSize() const { return mObjectIDs.size(); }
			bool IsEmpty() const { return mObjectIDs.empty(); }
			const std::set<unsigned int>& GetObjectIDs() const { return mObjectIDs; }

			void AddGoal(const unsigned int goalID) { mGoalIDs.insert(goalID); }
			const std::set<unsigned int>& GetGoals() const { return mGoalIDs; }

		private:
			std::set<unsigned int> mObjectIDs;  // unit member ID's
			std::set<unsigned int> mGoalIDs;
	};

	std::map<unsigned int, MGroup*> mGroups;
	std::map<unsigned int, MObject*> mObjects;

	DataTypeInfo cachedScalarData;
	DataTypeInfo cachedVectorData;
};

IPathModule* CALL_CONV GetPathModuleInstance(ICallOutHandler* icoh) { return (new CCPathModule(icoh)); }
void         CALL_CONV FreePathModuleInstance(IPathModule* m) { delete ((CCPathModule*) m); }

#endif
