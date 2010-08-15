#include <iostream>

#include "./CCPathModule.hpp"
#include "../../Math/vec3.hpp"
#include "../../Ext/ICallOutHandler.hpp"
#include "../../Sim/SimObjectDef.hpp"
#include "../../Sim/SimObjectState.hpp"
#include "../../System/ScopedTimer.hpp"

void CCPathModule::OnEvent(const IEvent* e) {
	switch (e->GetType()) {
		case EVENT_SIMOBJECT_CREATED: {
			const SimObjectCreatedEvent* ee = dynamic_cast<const SimObjectCreatedEvent*>(e);
			const unsigned int objectID = ee->GetObjectID();

			mObjects[objectID] = new MObject(coh->GetSimObjectDef(objectID));
			coh->SetSimObjectPhysicsUpdates(objectID, false);
		} break;

		case EVENT_SIMOBJECT_DESTROYED: {
			const SimObjectDestroyedEvent* ee = dynamic_cast<const SimObjectDestroyedEvent*>(e);
			const unsigned int objectID = ee->GetObjectID();

			DelObjectFromGroup(objectID);
			mObjects.erase(objectID);

			if (mObjects.empty()) {
				PFFG_ASSERT(mGroups.empty());

				// reset the group counter
				numGroupIDs = 0;
			}
		} break;

		case EVENT_SIMOBJECT_MOVEORDER: {
			const SimObjectMoveOrderEvent* ee = dynamic_cast<const SimObjectMoveOrderEvent*>(e);

			if (ee->GetQueued()) {
				// too complicated: if this order is queued, we would want
				// to preserve the remaining orders of the previous groups
				// that these objects were in and merge them
				//
				// possible solution: create as many singleton-groups as
				// the number of units in this order (very inefficient)
				//
				// for a queued order we also do NOT want multiple "sinks"
				// per group; only for immediate "line" formation orders
				// however, even a line order just consists of individual
				// movement commands, so multiple sinks are unnecessary
				//
				// for now, we define the "group has arrived" criterion
				// as "all members are within a predetermined threshold
				// range"
				//
				// [UpdateGroupPotentialField should get the goal cells
				// from a specific group, but how will we select them?]
				return;
			}

			const std::list<unsigned int>& objectIDs = ee->GetObjectIDs();
			const vec3f& goalPos = ee->GetGoalPos();

			// create a new group
			const unsigned int groupID = numGroupIDs++;

			MGroup* newGroup = new MGroup();
			mGroups[groupID] = newGroup;

			PFFG_ASSERT(newGroup != NULL);

			newGroup->SetGoal(mGrid.World2Cell(goalPos));
			mGrid.AddGroup(groupID);

			for (std::list<unsigned int>::const_iterator it = objectIDs.begin(); it != objectIDs.end(); ++it) {
				const unsigned int objectID = *it;

				PFFG_ASSERT(coh->IsValidSimObjectID(objectID));

				DelObjectFromGroup(objectID);
				AddObjectToGroup(groupID, objectID);

				// needed to show the proper movement line indicator
				WantedPhysicalState wps = coh->GetSimObjectWantedPhysicalState(objectID, true);
					wps.wantedPos = goalPos;
					wps.wantedDir = (goalPos - coh->GetSimObjectPosition(objectID)).norm();
					wps.wantedSpeed = 0.0f;
				coh->PushSimObjectWantedPhysicalState(objectID, wps, ee->GetQueued(), false);
			}
		} break;

		case EVENT_SIMOBJECT_COLLISION: {
		} break;

		default: {
		} break;
	}
}



void CCPathModule::Init() {
	printf("[CCPathModule::Init]\n");

	mGrid.Init(16, coh);
}

void CCPathModule::Update() {
	#ifdef CCPATHMODULE_PROFILE
	const static std::string s = "[CCPathModule::Update]";
	const unsigned int t = ScopedTimer::GetTaskTime(s);
	#endif

	{
		#ifdef CCPATHMODULE_PROFILE
		ScopedTimer timer(s);
		#endif

		typedef std::list<unsigned int> List;
		typedef std::list<unsigned int>::const_iterator ListIt;
		typedef std::set<unsigned int> Set;
		typedef std::set<unsigned int>::const_iterator SetIt;

		static std::vector<unsigned int> groupGoals(1, -1);

		List idleGroups;
		ListIt idleGroupsIt;

		// reset all grid-cells to the global-static state
		mGrid.Reset();

		// convert the crowd into a density field (rho)
		for (std::map<unsigned int, MObject*>::iterator it = mObjects.begin(); it != mObjects.end(); ++it) {
			const unsigned int objID = it->first;
			const vec3f& objPos = coh->GetSimObjectPosition(objID);
			const vec3f objVel =
				coh->GetSimObjectDirection(objID) *
				coh->GetSimObjectSpeed(objID);

			// NOTE:
			//   if objVel is a zero-vector, then avgVel will not change
			//   therefore the flow speed can stay zero in a region, so
			//   that *only* the topological speed determines the speed
			//   field there
			mGrid.AddDensityAndVelocity(objPos, objVel);
		}

		// now that we know the cumulative density per cell,
		// we can compute the average velocity field (v-bar)
		mGrid.ComputeAvgVelocity();

		for (std::map<unsigned int, MGroup*>::iterator it = mGroups.begin(); it != mGroups.end(); ++it) {
			const MGroup*      group         = it->second;
			const unsigned int groupID       = it->first;
			const unsigned int groupGoalCell = group->GetGoal();

			const Set& groupObjectIDs = group->GetObjectIDs();

			groupGoals[0] = groupGoalCell;

			// for each active group <groupID>, first construct the speed- and
			// unit-cost field (f and C); second, calculate the potential- and
			// gradient-fields (phi and delta-phi)
			//
			// NOTE: discomfort regarding this group can be computed here
			// NOTE: it might be possible to compute the speed- and cost-
			//       fields in the UpdateGroupPotentialField as cells are
			//       picked from the UNKNOWN set, saving N iterations
			mGrid.UpdateGroupPotentialField(groupID, groupGoals, groupObjectIDs);

			unsigned int numArrivedObjects = 0;

			// finally, update the locations of objects in this group ("advection")
			for (SetIt git = groupObjectIDs.begin(); git != groupObjectIDs.end(); ++git) {
				const unsigned int objectID = *git;
				const unsigned int objectCell = mGrid.World2Cell(coh->GetSimObjectPosition(objectID));

				if (objectCell == groupGoalCell) {
					numArrivedObjects += 1;
				} else {
					mGrid.UpdateSimObjectLocation(objectID);
				}
			}

			if (numArrivedObjects >= groupObjectIDs.size()) {
				// all units have arrived, mark the group for deletion
				idleGroups.push_back(groupID);
			}
		}

		for (idleGroupsIt = idleGroups.begin(); idleGroupsIt != idleGroups.end(); ++idleGroupsIt) {
			DelGroup(*idleGroupsIt);
		}

		// TODO: enforce minimum distance between objects
		// NOTE: should this be handled via EVENT_SIMOBJECT_COLLISION?
	}

	#ifdef CCPATHMODULE_PROFILE
	printf("%s time: %ums\n\n", s.c_str(), (ScopedTimer::GetTaskTime(s) - t));;
	#endif
}

void CCPathModule::Kill() {
	printf("[CCPathModule::Kill]\n");

	for (std::map<unsigned int, MGroup*>::const_iterator it = mGroups.begin(); it != mGroups.end(); ++it) {
		mGrid.DelGroup(it->first); delete it->second;
	}
	for (std::map<unsigned int, MObject*>::const_iterator it = mObjects.begin(); it != mObjects.end(); ++it) {
		delete it->second;
	}

	mGrid.Kill();
	mGroups.clear();
	mObjects.clear();
}






bool CCPathModule::DelObjectFromGroup(unsigned int objectID) {
	MObject* object = mObjects[objectID]; // already created
	MGroup* group = NULL;

	PFFG_ASSERT(object != NULL);

	typedef std::map<unsigned int, MGroup*> Map;
	typedef std::map<unsigned int, MGroup*>::iterator MapIt;

	MapIt git = mGroups.find(object->GetGroupID());

	if (git != mGroups.end()) {
		const unsigned int groupID = object->GetGroupID();

		group = (git->second);
		group->DelObject(objectID);

		if (group->IsEmpty()) {
			// old group is now empty, delete it
			mGrid.DelGroup(groupID);
			mGroups.erase(groupID);
			delete group;
		}

		object->SetGroupID(-1);
		return true;
	}

	// object was not in a group
	return false;
}

void CCPathModule::AddObjectToGroup(unsigned int groupID, unsigned int objectID) {
	MGroup* group = mGroups[groupID]; // already created
	MObject* object = mObjects[objectID];

	PFFG_ASSERT(group != NULL && object != NULL);

	group->AddObject(objectID);
	object->SetGroupID(groupID);
}

bool CCPathModule::DelGroup(unsigned int groupID) {
	std::map<unsigned int, MGroup*>::iterator git = mGroups.find(groupID);

	if (git == mGroups.end()) {
		return false;
	}

	const MGroup* group = git->second;
	const std::set<unsigned int>& gObjectIDs = group->GetObjectIDs();

	for (std::set<unsigned int>::const_iterator git = gObjectIDs.begin(); git != gObjectIDs.end(); ++git) {
		mObjects[*git]->SetGroupID(-1);
	}

	mGroups.erase(groupID);
	mGrid.DelGroup(groupID);
	delete group;
	return true;
}






unsigned int CCPathModule::GetScalarDataArraySizeX(unsigned int dataType) const {
	switch (dataType) {
		case Grid::DATATYPE_DENSITY:    { return mGrid.GetGridWidth(); } break;
		case Grid::DATATYPE_HEIGHT:     { return mGrid.GetGridWidth(); } break;
		case Grid::DATATYPE_DISCOMFORT: { return mGrid.GetGridWidth(); } break;
		case Grid::DATATYPE_SPEED:      { return mGrid.GetGridWidth(); } break;
		case Grid::DATATYPE_COST:       { return mGrid.GetGridWidth(); } break;
		case Grid::DATATYPE_POTENTIAL:  { return mGrid.GetGridWidth(); } break;
		default: {} break;
	}

	return 0;
}

unsigned int CCPathModule::GetScalarDataArraySizeZ(unsigned int dataType) const {
	switch (dataType) {
		case Grid::DATATYPE_DENSITY:    { return mGrid.GetGridHeight(); } break;
		case Grid::DATATYPE_HEIGHT:     { return mGrid.GetGridHeight(); } break;
		case Grid::DATATYPE_DISCOMFORT: { return mGrid.GetGridHeight(); } break;
		case Grid::DATATYPE_SPEED:      { return mGrid.GetGridHeight(); } break;
		case Grid::DATATYPE_COST:       { return mGrid.GetGridHeight(); } break;
		case Grid::DATATYPE_POTENTIAL:  { return mGrid.GetGridHeight(); } break;
		default: {} break;
	}

	return 0;
}

unsigned int CCPathModule::GetScalarDataArrayStride(unsigned int dataType) const {
	switch (dataType) {
		case Grid::DATATYPE_DENSITY:    { return              1; } break;
		case Grid::DATATYPE_HEIGHT:     { return              1; } break;
		case Grid::DATATYPE_DISCOMFORT: { return              1; } break;
		case Grid::DATATYPE_SPEED:      { return Grid::NUM_DIRS; } break;
		case Grid::DATATYPE_COST:       { return Grid::NUM_DIRS; } break;
		case Grid::DATATYPE_POTENTIAL:  { return              1; } break;
		default: {} break;
	}

	return 0;
}

const float* CCPathModule::GetScalarDataArray(unsigned int dataType, unsigned int groupID) const {
	switch (dataType) {
		case Grid::DATATYPE_DENSITY:    { return mGrid.GetDensityVisDataArray();           } break;
		case Grid::DATATYPE_HEIGHT:     { return mGrid.GetHeightVisDataArray();            } break;
		case Grid::DATATYPE_DISCOMFORT: { return mGrid.GetDiscomfortVisDataArray(groupID); } break;
		case Grid::DATATYPE_SPEED:      { return mGrid.GetSpeedVisDataArray(groupID);      } break;
		case Grid::DATATYPE_COST:       { return mGrid.GetCostVisDataArray(groupID);       } break;
		case Grid::DATATYPE_POTENTIAL:  { return mGrid.GetPotentialVisDataArray(groupID);  } break;
		default: {} break;
	}

	return NULL;
}



unsigned int CCPathModule::GetVectorDataArraySizeX(unsigned int dataType) const {
	switch (dataType) {
		case Grid::DATATYPE_HEIGHT_DELTA:    { return mGrid.GetGridWidth(); }
		case Grid::DATATYPE_VELOCITY_AVG:    { return mGrid.GetGridWidth(); }
		case Grid::DATATYPE_VELOCITY:        { return mGrid.GetGridWidth(); }
		case Grid::DATATYPE_POTENTIAL_DELTA: { return mGrid.GetGridWidth(); }
		default: {} break;
	}

	return 0;
}
unsigned int CCPathModule::GetVectorDataArraySizeZ(unsigned int dataType) const {
	switch (dataType) {
		case Grid::DATATYPE_HEIGHT_DELTA:    { return mGrid.GetGridHeight(); }
		case Grid::DATATYPE_VELOCITY_AVG:    { return mGrid.GetGridHeight(); }
		case Grid::DATATYPE_VELOCITY:        { return mGrid.GetGridHeight(); }
		case Grid::DATATYPE_POTENTIAL_DELTA: { return mGrid.GetGridHeight(); }
		default: {} break;
	}

	return 0;
}
unsigned int CCPathModule::GetVectorDataArrayStride(unsigned int dataType) const {
	switch (dataType) {
		case Grid::DATATYPE_HEIGHT_DELTA:    { return Grid::NUM_DIRS; }
		case Grid::DATATYPE_VELOCITY_AVG:    { return              1; }
		case Grid::DATATYPE_VELOCITY:        { return Grid::NUM_DIRS; }
		case Grid::DATATYPE_POTENTIAL_DELTA: { return Grid::NUM_DIRS; }
		default: {} break;
	}

	return 0;
}

const vec3f* CCPathModule::GetVectorDataArray(unsigned int dataType, unsigned int groupID) const {
	switch (dataType) {
		case Grid::DATATYPE_HEIGHT_DELTA:    { return mGrid.GetHeightDeltaVisDataArray();           } break;
		case Grid::DATATYPE_VELOCITY_AVG:    { return mGrid.GetVelocityAvgVisDataArray();           } break;
		case Grid::DATATYPE_VELOCITY:        { return mGrid.GetVelocityVisDataArray(groupID);       } break;
		case Grid::DATATYPE_POTENTIAL_DELTA: { return mGrid.GetPotentialDeltaVisDataArray(groupID); } break;
		default: {} break;
	}

	return NULL;
}
