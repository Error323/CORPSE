#ifndef PFFG_IPATH_MODULE_HDR
#define PFFG_IPATH_MODULE_HDR

#include <map>
#include <set>

#include "../System/IEngineModule.hpp"

#include "../Math/mat44fwd.hpp"
#include "../Math/vec3fwd.hpp"

#define DATATYPEINFO_CACHED {0, 0, 0, 0, 0, {NULL}, "", false, true};
#define DATATYPEINFO_RWRITE {0, 0, 0, 0, 0, {NULL}, "", false, false};

class IEvent;
class SimObjectDef;
class ICallOutHandler;
class IPathModule: public IEngineModule {
public:
	IPathModule(ICallOutHandler* icoh): coh(icoh) {}
	virtual ~IPathModule() {}

	virtual void OnEvent(const IEvent*) {}

	virtual void Init() {}
	virtual void Update() {}
	virtual void Kill() {}

	virtual ICallOutHandler* GetCallOutHandler() const { return coh; }

	struct DataTypeInfo {
		unsigned int type;
		unsigned int group;
		unsigned int sizex;
		unsigned int sizey;
		unsigned int stride;
		union {
			const float* fdata;
			const vec3f* vdata;
		};
		const char* name;
		bool global;
		bool cached;
	};

	virtual bool GetScalarDataTypeInfo(DataTypeInfo*) const = 0;
	virtual bool GetVectorDataTypeInfo(DataTypeInfo*) const = 0;
	virtual unsigned int GetNumScalarDataTypes() const = 0;
	virtual unsigned int GetNumVectorDataTypes() const = 0;

	virtual unsigned int GetNumGroupIDs() const { return mGroups.size(); }
	virtual unsigned int GetGroupIDs(unsigned int* array, unsigned int size) const {
		unsigned int n = 0;

		std::map<unsigned int, MGroup*>::const_iterator it;
		for (it = mGroups.begin(); it != mGroups.end() && n < size; ++it) {
			array[n++] = it->first;
		}

		return n;
	}

protected:
	ICallOutHandler* coh;

	struct MGroup;
	struct MObject;
	typedef std::set<unsigned int> Set;
	typedef std::set<unsigned int>::const_iterator SetIt;
	typedef std::map<unsigned int, MGroup*> GroupMap;
	typedef std::map<unsigned int, MGroup*>::iterator GroupMapIt;
	typedef std::map<unsigned int, MObject*> ObjectMap;
	typedef std::map<unsigned int, MObject*>::iterator ObjectMapIt;

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

	// running counter used to assign ID's to new groups
	// (not the actual number of currently active groups)
	unsigned int numGroupIDs;
};

// engine gets and frees a module instance through these; implemented library-side
extern "C" SLIB_EXPORT   IPathModule*   CALL_CONV   GetPathModuleInstance(ICallOutHandler*);
extern "C" SLIB_EXPORT   void           CALL_CONV   FreePathModuleInstance(IPathModule*);

#endif
