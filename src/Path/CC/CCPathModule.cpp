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

			// handling queued orders is too complicated: we would want
			// to preserve the remaining orders of the previous groups
			// that these objects were in and merge them
			//
			// possible solution: create as many singleton-groups as
			// the number of units in this order (very inefficient)
			//
			// for a queued order we also do NOT want multiple sinks
			// per group; only for immediate "line" formation orders
			// however, even a line order just consists of individual
			// movement commands, so multiple sinks are unnecessary?
			//
			// a point-move order involving multiple units could be
			// implemented by adding as many potential-field sinks,
			// but what should the arrival-check look like in that
			// case? "all units within range of at least one goal"?
			// (queued orders are handled like this now; note that
			// there is *no* guarantee that any unit will arrive at
			// its own "preferred" individual goal offset)
			//
			// for now, we define the "group has arrived" criterion
			// as "all members are within a predetermined threshold
			// range of the group's single goal-cell"
			//
			// [UpdateGroupPotentialField should get the goal cells
			// from a specific group, but how will we select them?]

			// create a new group
			const unsigned int groupID = numGroupIDs++;

			const std::list<unsigned int>& objectIDs = ee->GetObjectIDs();
			const vec3f& goalPos = ee->GetGoalPos();

			vec3f groupPos;

			MGroup* newGroup = new MGroup();
			mGroups[groupID] = newGroup;

			PFFG_ASSERT(newGroup != NULL);

			if (ee->GetQueued()) {
				// get the geometric average position
				for (std::list<unsigned int>::const_iterator it = objectIDs.begin(); it != objectIDs.end(); ++it) {
					groupPos += coh->GetSimObjectPosition(*it);
				}

				groupPos /= objectIDs.size();
			} else {
				newGroup->AddGoal(mGrid.WorldPosToCellID(goalPos));
			}

			for (std::list<unsigned int>::const_iterator it = objectIDs.begin(); it != objectIDs.end(); ++it) {
				const unsigned int objectID = *it;
				const vec3f& objectPos = coh->GetSimObjectPosition(objectID);

				PFFG_ASSERT(coh->IsValidSimObjectID(objectID));

				DelObjectFromGroup(objectID);
				AddObjectToGroup(groupID, objectID);

				// needed to show the proper movement line indicator
				WantedPhysicalState wps = coh->GetSimObjectWantedPhysicalState(objectID, true);

				if (ee->GetQueued()) {
					wps.wantedPos   = goalPos + (objectPos - groupPos);
					wps.wantedDir   = (wps.wantedPos - objectPos).norm();
					wps.wantedSpeed = coh->GetSimObjectDef(objectID)->GetMaxForwardSpeed();

					// World2Cell clamps the position via World2Grid
					newGroup->AddGoal(mGrid.WorldPosToCellID(wps.wantedPos));
				} else {
					wps.wantedPos   = goalPos;
					wps.wantedDir   = (goalPos - coh->GetSimObjectPosition(objectID)).norm();
					wps.wantedSpeed = coh->GetSimObjectDef(objectID)->GetMaxForwardSpeed();
				}

				coh->PushSimObjectWantedPhysicalState(objectID, wps, false, false);
				coh->SetSimObjectPhysicsUpdates(objectID, false);
			}

			mGrid.AddGroup(groupID);
		} break;

		case EVENT_SIMOBJECT_COLLISION: {
		} break;

		default: {
		} break;
	}
}



void CCPathModule::Init() {
	printf("[CCPathModule::Init]\n");

	mGrid.Init(8, coh);
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

		UpdateGrid();
		UpdateGroups();

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






void CCPathModule::UpdateGrid() {
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
}

void CCPathModule::UpdateGroups() {
	typedef std::list<unsigned int> List;
	typedef std::list<unsigned int>::const_iterator ListIt;
	typedef std::set<unsigned int> Set;
	typedef std::set<unsigned int>::const_iterator SetIt;

	List idleGroups;
	ListIt idleGroupsIt;

	for (std::map<unsigned int, MGroup*>::iterator it = mGroups.begin(); it != mGroups.end(); ++it) {
		const MGroup*      group         = it->second;
		const unsigned int groupID       = it->first;

		const Set& groupGoalIDs = group->GetGoals();
		const Set& groupObjectIDs = group->GetObjectIDs();

		// for each active group <groupID>, first construct the speed- and
		// unit-cost field (f and C); second, calculate the potential- and
		// gradient-fields (phi and delta-phi)
		//
		// NOTE: discomfort regarding this group can be computed here
		// NOTE: it might be possible to compute the speed- and cost-
		//       fields in the UpdateGroupPotentialField as cells are
		//       picked from the UNKNOWN set, saving N iterations
		mGrid.UpdateGroupPotentialField(groupID, groupGoalIDs, groupObjectIDs);


		unsigned int numArrivedObjects = 0;

		// finally, update the locations of objects in this group ("advection")
		// (the complexity of this is O(M * K) with M the number of units and K
		// the number of goals)
		for (SetIt goit = groupObjectIDs.begin(); goit != groupObjectIDs.end(); ++goit) {
			const unsigned int objectID = *goit;
			const unsigned int objectCellID = mGrid.WorldPosToCellID(coh->GetSimObjectPosition(objectID));

			if (mObjects[objectID]->HasArrived()) {
				numArrivedObjects += 1;
				continue;
			}

			mGrid.UpdateSimObjectLocation(objectID, objectCellID);

			const vec3f& objectPos = coh->GetSimObjectPosition(objectID);
			const vec3f& objectDir = coh->GetSimObjectDirection(objectID);

			for (SetIt ggit = groupGoalIDs.begin(); ggit != groupGoalIDs.end(); ++ggit) {
				const Grid::Cell* goalCell = mGrid.GetCell(*ggit);
				const vec3f goalPos = mGrid.GetCellPos(goalCell);

				if ((objectPos - goalPos).sqLen2D() < (mGrid.GetSquareSize() * mGrid.GetSquareSize())) {
					mObjects[objectID]->SetArrived(true);

					WantedPhysicalState wps = coh->GetSimObjectWantedPhysicalState(objectID, true);

					// just come to a halt if close to some goal cell
					// (by letting the engine stop the unit's movement)
					wps.wantedPos   = objectPos;
					wps.wantedDir   = objectDir;
					wps.wantedSpeed = 0.0f;

					coh->PushSimObjectWantedPhysicalState(objectID, wps, false, true);
					coh->SetSimObjectPhysicsUpdates(objectID, true);
					break;
				}
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
	object->SetArrived(false);
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






bool CCPathModule::GetScalarDataTypeInfo(DataTypeInfo* i, unsigned int groupID) const {
	bool ret = true;

	i->sizex = mGrid.GetGridWidth();
	i->sizey = mGrid.GetGridHeight();

	switch (i->type) {
		case Grid::DATATYPE_DENSITY:    { i->fdata = mGrid.GetDensityVisDataArray();           i->stride =              1; i->global = true;  i->name =    "DENSITY"; } break;
		case Grid::DATATYPE_HEIGHT:     { i->fdata = mGrid.GetHeightVisDataArray();            i->stride =              1; i->global = true;  i->name =     "HEIGHT"; } break;
		case Grid::DATATYPE_DISCOMFORT: { i->fdata = mGrid.GetDiscomfortVisDataArray(groupID); i->stride =              1; i->global = false; i->name = "DISCOMFORT"; } break;
		case Grid::DATATYPE_SPEED:      { i->fdata = mGrid.GetSpeedVisDataArray(groupID);      i->stride = Grid::NUM_DIRS; i->global = false; i->name =      "SPEED"; } break;
		case Grid::DATATYPE_COST:       { i->fdata = mGrid.GetCostVisDataArray(groupID);       i->stride = Grid::NUM_DIRS; i->global = false; i->name =       "COST"; } break;
		case Grid::DATATYPE_POTENTIAL:  { i->fdata = mGrid.GetPotentialVisDataArray(groupID);  i->stride =              1; i->global = false; i->name =  "POTENTIAL"; } break;
		default: { ret = false; } break;
	}

	return ret;
}

bool CCPathModule::GetVectorDataTypeInfo(DataTypeInfo* i, unsigned int groupID) const {
	bool ret = true;

	i->sizex = mGrid.GetGridWidth();
	i->sizey = mGrid.GetGridHeight();

	switch (i->type) {
		case Grid::DATATYPE_HEIGHT_DELTA:    { i->vdata = mGrid.GetHeightDeltaVisDataArray();           i->stride = Grid::NUM_DIRS; i->global = true;  i->name =    "HEIGHT_DELTA"; } break;
		case Grid::DATATYPE_VELOCITY_AVG:    { i->vdata = mGrid.GetVelocityAvgVisDataArray();           i->stride =              1; i->global = true;  i->name =    "VELOCITY_AVG"; } break;
		case Grid::DATATYPE_VELOCITY:        { i->vdata = mGrid.GetVelocityVisDataArray(groupID);       i->stride = Grid::NUM_DIRS; i->global = false; i->name =        "VELOCITY"; } break;
		case Grid::DATATYPE_POTENTIAL_DELTA: { i->vdata = mGrid.GetPotentialDeltaVisDataArray(groupID); i->stride = Grid::NUM_DIRS; i->global = false; i->name = "POTENTIAL_DELTA"; } break;
		default: { ret = false; } break;
	}

	return ret;
}
