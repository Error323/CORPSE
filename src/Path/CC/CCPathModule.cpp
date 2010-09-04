#include <iostream>

#include "./CCPathModule.hpp"
#include "../../Math/vec3.hpp"
#include "../../Ext/ICallOutHandler.hpp"
#include "../../Sim/SimObjectDef.hpp"
#include "../../Sim/SimObjectState.hpp"
#include "../../System/ScopedTimer.hpp"

#define GRID_DOWNSCALE_FACTOR         8
#define MINIMUM_DISTANCE_ENFORCEMENT  1
#define PREDICTIVE_DISCOMFORT_FRAMES 10

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
				newGroup->AddGoal(mGrid.GetCellIndex1D(goalPos));
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
					wps.wantedDir   = (wps.wantedPos - objectPos).norm3D();
					wps.wantedSpeed = coh->GetSimObjectDef(objectID)->GetMaxForwardSpeed();

					// World2Cell clamps the position via World2Grid
					newGroup->AddGoal(mGrid.GetCellIndex1D(wps.wantedPos));
				} else {
					wps.wantedPos   = goalPos;
					wps.wantedDir   = (goalPos - coh->GetSimObjectPosition(objectID)).norm3D();
					wps.wantedSpeed = coh->GetSimObjectDef(objectID)->GetMaxForwardSpeed();
				}

				coh->PushSimObjectWantedPhysicalState(objectID, wps, false, false);
				coh->SetSimObjectPhysicsUpdates(objectID, false);
			}

			mGrid.AddGroup(groupID);
		} break;

		case EVENT_SIMOBJECT_COLLISION: {
			#if (MINIMUM_DISTANCE_ENFORCEMENT == 1)
			const SimObjectCollisionEvent* ee = dynamic_cast<const SimObjectCollisionEvent*>(e);

			const unsigned int colliderID = ee->GetColliderID();
			const unsigned int collideeID = ee->GetCollideeID();

			const vec3f& colliderPos = coh->GetSimObjectPosition(colliderID);
			const vec3f& collideePos = coh->GetSimObjectPosition(collideeID);

			const float colliderRadius = coh->GetSimObjectRadius(colliderID);
			const float collideeRadius = coh->GetSimObjectRadius(collideeID);

			const vec3f separationVec = colliderPos - collideePos;
			const float separationMin = (colliderRadius + collideeRadius) * (colliderRadius + collideeRadius);

			// enforce minimum distance between objects
			if ((separationVec.sqLen3D() - separationMin) < 0.0f) {
				const float dst = (separationVec.len3D());
				const vec3f dir = (separationVec / dst);
				const vec3f dif = (dir * (((colliderRadius + collideeRadius) - dst) * 0.5f));

				coh->SetSimObjectRawPosition(colliderID, colliderPos + dif);
				coh->SetSimObjectRawPosition(collideeID, collideePos - dif);
			}
			#endif
		} break;

		default: {
		} break;
	}
}



void CCPathModule::Init() {
	printf("[CCPathModule::Init]\n");
	printf("\tGRID_DOWNSCALE_FACTOR:        %d\n", GRID_DOWNSCALE_FACTOR);
	printf("\tMINIMUM_DISTANCE_ENFORCEMENT: %d\n", MINIMUM_DISTANCE_ENFORCEMENT);
	printf("\tPREDICTIVE_DISCOMFORT_FRAMES: %d\n", PREDICTIVE_DISCOMFORT_FRAMES);

	// NOTE:
	//     GRID_DOWNSCALE_FACTOR needs to be set such that the
	//     *radius* of the largest agent model (in world space)
	//     is <= the center-to-edge distance of a grid cell (in
	//     world space), otherwise the TCP06 density conversion
	//     (involving cells ABCD) is ill-defined
	//     (alternative conversion schemes are allowed however)
	mGrid.Init(GRID_DOWNSCALE_FACTOR, coh);

	static const DataTypeInfo scalarData = DATATYPEINFO_CACHED; cachedScalarData = scalarData;
	static const DataTypeInfo vectorData = DATATYPEINFO_CACHED; cachedVectorData = vectorData;
}

void CCPathModule::Update() {
	static unsigned int frame = 0;

	#ifdef CCPATHMODULE_PROFILE
	const static std::string s = "[CCPathModule::Update]";
	const unsigned int t = ScopedTimer::GetTaskTime(s);
	#endif

	{
		#ifdef CCPATHMODULE_PROFILE
		ScopedTimer timer(s);
		#endif

		UpdateGrid((frame == 0) || ((frame % mGrid.GetUpdateInterval()) == 0));
		UpdateGroups((frame == 0) || ((frame % mGrid.GetUpdateInterval()) == 0));
	}

	#ifdef CCPATHMODULE_PROFILE
	printf("%s time: %ums\n\n", s.c_str(), (ScopedTimer::GetTaskTime(s) - t));;
	#endif

	frame += 1;
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






void CCPathModule::UpdateGrid(bool isUpdateFrame) {
	if (isUpdateFrame) {
		// reset all grid-cells to the global-static state
		mGrid.Reset();

		// convert the crowd into a density field (rho)
		for (std::map<unsigned int, MObject*>::iterator it = mObjects.begin(); it != mObjects.end(); ++it) {
			const unsigned int objID = (it->first);
			const SimObjectDef* objDef = (it->second)->GetDef();
			const vec3f& objPos = coh->GetSimObjectPosition(objID);
			const vec3f objVel =
				coh->GetSimObjectDirection(objID) *
				coh->GetSimObjectSpeed(objID);
			const float objRad = coh->GetSimObjectRadius(objID);

			// NOTE:
			//   if objVel is a zero-vector, then avgVel will not change
			//   therefore the flow speed can stay zero in a region, so
			//   that *only* the topological speed determines the speed
			//   field there
			mGrid.AddDensity(objPos, objVel, objRad);

			#if (PREDICTIVE_DISCOMFORT_FRAMES > 0)
			// NOTE:
			//   combine this with AddDensity?
			//
			//   adding discomfort in front of every unit just results
			//   in more self-obstructions, unless the discomfort-field
			//   is per-group and discomfort for a unit in group <g> is
			//   only registered on the fields of the groups != <g> (but
			//   then units within the same group would lack foresight)
			//
			//   the amount of lookahead should depend on the object's
			//   maximum speed and radius (wrt. the cell-size) instead
			//   of a fixed value
			mGrid.AddDiscomfort(objPos, objVel, objRad, PREDICTIVE_DISCOMFORT_FRAMES, (mGrid.GetSquareSize() / objDef->GetMaxForwardSpeed()));
			#endif
		}

		// now that we know the cumulative density per cell,
		// we can compute the average velocity field (v-bar)
		mGrid.ComputeAvgVelocity();
	}
}

void CCPathModule::UpdateGroups(bool isUpdateFrame) {
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

		if (isUpdateFrame) {
			// for each active group <groupID>, first construct the speed- and
			// unit-cost field (f and C); second, calculate the potential- and
			// gradient-fields (phi and delta-phi)
			//
			// NOTE: discomfort regarding this group can be computed here
			// NOTE: it might be possible to compute the speed- and cost-
			//       fields in the UpdateGroupPotentialField as cells are
			//       picked from the UNKNOWN set, saving N iterations
			mGrid.UpdateGroupPotentialField(groupID, groupGoalIDs, groupObjectIDs);
		}


		unsigned int numArrivedObjects = 0;

		// finally, update the locations of objects in this group ("advection")
		// (the complexity of this is O(M * K) with M the number of units and K
		// the number of goals)
		for (SetIt goit = groupObjectIDs.begin(); goit != groupObjectIDs.end(); ++goit) {
			const unsigned int objectID = *goit;
			const unsigned int objectCellID = mGrid.GetCellIndex1D(coh->GetSimObjectPosition(objectID));

			if (mObjects[objectID]->HasArrived()) {
				numArrivedObjects += 1;
				continue;
			}

			mGrid.UpdateSimObjectLocation(objectID, objectCellID);

			const vec3f& objectPos = coh->GetSimObjectPosition(objectID);
			const vec3f& objectDir = coh->GetSimObjectDirection(objectID);

			for (SetIt ggit = groupGoalIDs.begin(); ggit != groupGoalIDs.end(); ++ggit) {
				const Grid::Cell* goalCell = mGrid.GetCell(*ggit);
				const vec3f& goalPos = mGrid.GetCellMidPos(goalCell);

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






bool CCPathModule::GetScalarDataTypeInfo(DataTypeInfo* i) const {
	bool ret = true;

	if (!i->cached) {
		i->sizex = mGrid.GetGridWidth();
		i->sizey = mGrid.GetGridHeight();

		switch (i->type) {
			case Grid::DATATYPE_DENSITY:    { i->fdata = mGrid.GetDensityVisDataArray();            i->stride =              1; i->global = true;  i->name =    "DENSITY"; } break;
			case Grid::DATATYPE_HEIGHT:     { i->fdata = mGrid.GetHeightVisDataArray();             i->stride =              1; i->global = true;  i->name =     "HEIGHT"; } break;
			case Grid::DATATYPE_SPEED:      { i->fdata = mGrid.GetSpeedVisDataArray(i->group);      i->stride = Grid::NUM_DIRS; i->global = false; i->name =      "SPEED"; } break;
			case Grid::DATATYPE_COST:       { i->fdata = mGrid.GetCostVisDataArray(i->group);       i->stride = Grid::NUM_DIRS; i->global = false; i->name =       "COST"; } break;
			case Grid::DATATYPE_POTENTIAL:  { i->fdata = mGrid.GetPotentialVisDataArray(i->group);  i->stride =              1; i->global = false; i->name =  "POTENTIAL"; } break;
			default: { ret = false; } break;
		}

		// HACK: we don't want to change the interface
		CCPathModule* m = const_cast<CCPathModule*>(this);
		m->cachedScalarData = *i;
		m->cachedScalarData.cached = true;
	} else {
		*i = cachedScalarData;
	}

	return ret;
}

bool CCPathModule::GetVectorDataTypeInfo(DataTypeInfo* i) const {
	bool ret = true;

	if (!i->cached) {
		i->sizex = mGrid.GetGridWidth();
		i->sizey = mGrid.GetGridHeight();

		switch (i->type) {
			case Grid::DATATYPE_DISCOMFORT:      { i->vdata = mGrid.GetDiscomfortVisDataArray();             i->stride =              1; i->global = true;  i->name =      "DISCOMFORT"; } break;
			case Grid::DATATYPE_HEIGHT_DELTA:    { i->vdata = mGrid.GetHeightDeltaVisDataArray();            i->stride = Grid::NUM_DIRS; i->global = true;  i->name =    "HEIGHT_DELTA"; } break;
			case Grid::DATATYPE_VELOCITY_AVG:    { i->vdata = mGrid.GetVelocityAvgVisDataArray();            i->stride =              1; i->global = true;  i->name =    "VELOCITY_AVG"; } break;
			case Grid::DATATYPE_VELOCITY:        { i->vdata = mGrid.GetVelocityVisDataArray(i->group);       i->stride = Grid::NUM_DIRS; i->global = false; i->name =        "VELOCITY"; } break;
			case Grid::DATATYPE_POTENTIAL_DELTA: { i->vdata = mGrid.GetPotentialDeltaVisDataArray(i->group); i->stride = Grid::NUM_DIRS; i->global = false; i->name = "POTENTIAL_DELTA"; } break;
			default: { ret = false; } break;
		}

		CCPathModule* m = const_cast<CCPathModule*>(this);
		m->cachedVectorData = *i;
		m->cachedVectorData.cached = true;
	} else {
		*i = cachedVectorData;
	}

	return ret;
}
