#include <iostream>

#include "./CCPathModule.hpp"
#include "../../Math/vec3.hpp"
#include "../../Ext/ICallOutHandler.hpp"
#include "../../Sim/SimObjectDef.hpp"
#include "../../Sim/SimObjectState.hpp"
#include "../../System/ScopedTimer.hpp"

void PathModule::OnEvent(const IEvent* e) {
	switch (e->GetType()) {
		case EVENT_SIMOBJECT_CREATED: {
			const SimObjectCreatedEvent* ee = dynamic_cast<const SimObjectCreatedEvent*>(e);
			const unsigned int objectID = ee->GetObjectID();

			mSimObjectIDs[objectID] = coh->GetSimObjectDef(objectID);
		} break;

		case EVENT_SIMOBJECT_DESTROYED: {
			const SimObjectDestroyedEvent* ee = dynamic_cast<const SimObjectDestroyedEvent*>(e);
			const unsigned int objectID = ee->GetObjectID();

			mSimObjectIDs.erase(objectID);
			DelObjectFromGroup(objectID);

			if (mSimObjectIDs.empty()) {
				PFFG_ASSERT(mObjectGroupIDs.empty());
				PFFG_ASSERT(mObjectGroups.empty());

				// reset the group counter
				numGroupIDs = 0;
			}
		} break;

		case EVENT_SIMOBJECT_MOVEORDER: {
			const SimObjectMoveOrderEvent* ee = dynamic_cast<const SimObjectMoveOrderEvent*>(e);

			const std::list<unsigned int>& objectIDs = ee->GetObjectIDs();
			const vec3f& goalPos = ee->GetGoalPos();

			// create a new group
			const unsigned int groupID = numGroupIDs++;

			for (std::list<unsigned int>::const_iterator it = objectIDs.begin(); it != objectIDs.end(); ++it) {
				const unsigned int objID = *it;

				PFFG_ASSERT(coh->IsValidSimObjectID(objID));

				DelObjectFromGroup(objID);
				AddObjectToGroup(objID, groupID);
			}

			// NOTE: how should these be cleaned up when the group arrives?
			mGoals[groupID].push_back(mGrid.World2Cell(goalPos));
		} break;

		case EVENT_SIMOBJECT_COLLISION: {
		} break;

		default: {
		} break;
	}
}

void PathModule::Init() {
	printf("[CCPathModule::Init]\n");
	mGrid.Init(8, coh);
}

void PathModule::Update() {
	#ifdef CCPATHMODULE_PROFILE
	static const std::string s = "[CCPathModule::Update]";
	const unsigned int t = ScopedTimer::GetTaskTime(s);
	#endif

	{
		#ifdef CCPATHMODULE_PROFILE
		ScopedTimer timer(s);
		#endif

		std::map<unsigned int, const SimObjectDef*>::iterator simObjectIt;
		std::map<unsigned int, std::set<unsigned int> >::iterator objectGroupIt;
		std::set<unsigned int>::iterator objectIDsIt;

		// reset all the cells in the grid
		mGrid.Reset();

		// convert the crowd into a density field (rho)
		for (simObjectIt = mSimObjectIDs.begin(); simObjectIt != mSimObjectIDs.end(); ++simObjectIt) {
			const unsigned int objID = simObjectIt->first;
			const vec3f& objPos = coh->GetSimObjectPosition(objID);
			const vec3f objVel =
				coh->GetSimObjectDirection(objID) *
				coh->GetSimObjectCurrentForwardSpeed(objID);

			mGrid.AddDensityAndVelocity(objPos, objVel);
		}

		// now that we know the cumulative density per cell,
		// we can compute the average velocity field (v-bar)
		mGrid.ComputeAvgVelocity();

		for (objectGroupIt = mObjectGroups.begin(); objectGroupIt != mObjectGroups.end(); ++objectGroupIt) {
			const unsigned int groupID = objectGroupIt->first;
			const std::set<unsigned int>& groupObjectIDs = objectGroupIt->second;

			// for each active group <groupID>, first construct the speed- and
			// unit-cost field (f and C); second, calculate the potential- and
			// gradient-fields (phi and delta-phi)
			//
			// NOTE: this first resets all the group-related variables
			// NOTE: discomfort regarding this group can be computed here
			// NOTE: it might be possible to compute the speedfield and unit-
			//        costfield in the UpdateGroupPotentialField as cells 
			//        are picked from the UNKNOWN set, saving N iterations
			// NOTE: this should get the goal cells from a specific group,
			//       but how will we select them?
			mGrid.UpdateGroupPotentialField(groupID, mGoals[groupID], groupObjectIDs);

			// finally, update the locations of objects in this group ("advection")
			for (objectIDsIt = groupObjectIDs.begin(); objectIDsIt != groupObjectIDs.end(); ++objectIDsIt) {
				mGrid.UpdateSimObjectLocation(*objectIDsIt);
			}
		}

		// enforce minimum distance between objects
		// NOTE: should this be handled via EVENT_SIMOBJECT_COLLISION?
	}

	#ifdef CCPATHMODULE_PROFILE
	printf("%s time: %ums\n", s.c_str(), (ScopedTimer::GetTaskTime(s) - t));;
	printf("\n");
	#endif
}

void PathModule::Kill() {
	printf("[CCPathModule::Kill]\n");
	mGrid.Kill(mObjectGroups);
}



void PathModule::AddObjectToGroup(unsigned int objID, unsigned int groupID) {
	if (mObjectGroups.find(groupID) == mObjectGroups.end()) {
		mObjectGroups[groupID] = std::set<unsigned int>();
		mGrid.AddGroup(groupID);
	}

	if (mGoals.find(groupID) == mGoals.end()) {
		mGoals[groupID] = std::vector<unsigned int>();
	}

	mObjectGroupIDs[objID] = groupID;
	mObjectGroups[groupID].insert(objID);
}

bool PathModule::DelObjectFromGroup(unsigned int objID) {
	if (mObjectGroupIDs.find(objID) != mObjectGroupIDs.end()) {
		const unsigned int groupID = mObjectGroupIDs[objID];

		mObjectGroupIDs.erase(objID);
		mObjectGroups[groupID].erase(objID);

		if (mObjectGroups[groupID].empty()) {
			// old group is now empty, delete it
			mObjectGroups.erase(groupID);
			mGoals[groupID].clear();
			mGoals.erase(groupID);
			mGrid.DelGroup(groupID);
		}

		return true;
	}

	// object was not in a group
	return false;
}






unsigned int PathModule::GetScalarDataArraySizeX(unsigned int dataType) const {
	switch (dataType) {
		case DATATYPE_DENSITY:    { return mGrid.GetGridWidth(); } break;
		case DATATYPE_HEIGHT:     { return mGrid.GetGridWidth(); } break;
		case DATATYPE_DISCOMFORT: { return mGrid.GetGridWidth(); } break;
		case DATATYPE_SPEED:      { return mGrid.GetGridWidth(); } break;
		case DATATYPE_COST:       { return mGrid.GetGridWidth(); } break;
		case DATATYPE_POTENTIAL:  { return mGrid.GetGridWidth(); } break;
		default: {} break;
	}

	return 0;
}

unsigned int PathModule::GetScalarDataArraySizeZ(unsigned int dataType) const {
	switch (dataType) {
		case DATATYPE_DENSITY:    { return mGrid.GetGridHeight(); } break;
		case DATATYPE_HEIGHT:     { return mGrid.GetGridHeight(); } break;
		case DATATYPE_DISCOMFORT: { return mGrid.GetGridHeight(); } break;
		case DATATYPE_SPEED:      { return mGrid.GetGridHeight(); } break;
		case DATATYPE_COST:       { return mGrid.GetGridHeight(); } break;
		case DATATYPE_POTENTIAL:  { return mGrid.GetGridHeight(); } break;
		default: {} break;
	}

	return 0;
}

unsigned int PathModule::GetScalarDataArrayStride(unsigned int dataType) const {
	switch (dataType) {
		case DATATYPE_DENSITY:    { return                    1; } break;
		case DATATYPE_HEIGHT:     { return                    1; } break;
		case DATATYPE_DISCOMFORT: { return                    1; } break;
		case DATATYPE_SPEED:      { return Grid::NUM_DIRECTIONS; } break;
		case DATATYPE_COST:       { return Grid::NUM_DIRECTIONS; } break;
		case DATATYPE_POTENTIAL:  { return                    1; } break;
		default: {} break;
	}

	return 0;
}

const float* PathModule::GetScalarDataArray(unsigned int dataType, unsigned int groupID) const {
	switch (dataType) {
		case DATATYPE_DENSITY:    { return mGrid.GetDensityVisDataArray();           } break;
		case DATATYPE_HEIGHT:     { return mGrid.GetHeightVisDataArray();            } break;
		case DATATYPE_DISCOMFORT: { return mGrid.GetDiscomfortVisDataArray(groupID); } break;
		case DATATYPE_SPEED:      { return mGrid.GetSpeedVisDataArray(groupID);      } break;
		case DATATYPE_COST:       { return mGrid.GetCostVisDataArray(groupID);       } break;
		case DATATYPE_POTENTIAL:  { return mGrid.GetPotentialVisDataArray(groupID);  } break;
		default: {} break;
	}

	return NULL;
}



unsigned int PathModule::GetVectorDataArraySizeX(unsigned int dataType) const {
	switch (dataType) {
		case DATATYPE_HEIGHT_DELTA:    { return mGrid.GetGridWidth(); }
		case DATATYPE_VELOCITY_AVG:    { return mGrid.GetGridWidth(); }
		case DATATYPE_VELOCITY:        { return mGrid.GetGridWidth(); }
		case DATATYPE_POTENTIAL_DELTA: { return mGrid.GetGridWidth(); }
		default: {} break;
	}

	return 0;
}
unsigned int PathModule::GetVectorDataArraySizeZ(unsigned int dataType) const {
	switch (dataType) {
		case DATATYPE_HEIGHT_DELTA:    { return mGrid.GetGridHeight(); }
		case DATATYPE_VELOCITY_AVG:    { return mGrid.GetGridHeight(); }
		case DATATYPE_VELOCITY:        { return mGrid.GetGridHeight(); }
		case DATATYPE_POTENTIAL_DELTA: { return mGrid.GetGridHeight(); }
		default: {} break;
	}

	return 0;
}
unsigned int PathModule::GetVectorDataArrayStride(unsigned int dataType) const {
	switch (dataType) {
		case DATATYPE_HEIGHT_DELTA:    { return Grid::NUM_DIRECTIONS; }
		case DATATYPE_VELOCITY_AVG:    { return                    1; }
		case DATATYPE_VELOCITY:        { return Grid::NUM_DIRECTIONS; }
		case DATATYPE_POTENTIAL_DELTA: { return Grid::NUM_DIRECTIONS; }
		default: {} break;
	}

	return 0;
}

const vec3f* PathModule::GetVectorDataArray(unsigned int dataType, unsigned int groupID) const {
	switch (dataType) {
		case DATATYPE_HEIGHT_DELTA:    { return mGrid.GetHeightDeltaVisDataArray();           } break;
		case DATATYPE_VELOCITY_AVG:    { return mGrid.GetVelocityAvgVisDataArray();           } break;
		case DATATYPE_VELOCITY:        { return mGrid.GetVelocityVisDataArray(groupID);       } break;
		case DATATYPE_POTENTIAL_DELTA: { return mGrid.GetPotentialDeltaVisDataArray(groupID); } break;
		default: {} break;
	}

	return NULL;
}
