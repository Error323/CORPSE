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
				// const vec3f& objPos = coh->GetSimObjectPosition(objID);

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
	std::cout << "[CCPathModule::Init]" << std::endl;
	mGrid.Init(8, coh);
}

void PathModule::Update() {
	static const std::string s = "[CCPathModule::Update]";
	#ifdef CCPATHMODULE_PROFILE
	const unsigned int t = ScopedTimer::GetTaskTime(s);
	#endif

	{
		#ifdef CCPATHMODULE_PROFILE
		ScopedTimer timer(s);
		#endif

		std::map<unsigned int, const SimObjectDef*>::iterator simObjectIt;
		std::map<unsigned int, std::set<unsigned int> >::iterator objectGroupIt;
		std::set<unsigned int>::iterator goalCellIt;

		// Reset all the cells in the grid
		mGrid.Reset();

		// Convert the crowd into a density field
		for (simObjectIt = mSimObjectIDs.begin(); simObjectIt != mSimObjectIDs.end(); ++simObjectIt) {
			const unsigned int objID = simObjectIt->first;
			const vec3f& objPos = coh->GetSimObjectPosition(objID);
			const vec3f objVel =
				coh->GetSimObjectDirection(objID) *
				coh->GetSimObjectCurrentForwardSpeed(objID);

			mGrid.AddDensityAndVelocity(objPos, objVel);
		}

		// Now that we know the cumulative density per cell,
		// we can compute the average velocity field
		mGrid.ComputeAvgVelocity();

		for (objectGroupIt = mObjectGroups.begin(); objectGroupIt != mObjectGroups.end(); ++objectGroupIt) {
			// For each group, construct the speed field and the unit cost field
			// Note1: This first resets all the group-related variables
			// Note2: Discomfort regarding this group can be computed here
			// Note3: It might be possible to compute the speedfield and unit-
			//        costfield in the UpdateGroupPotentialField as cells 
			//        are picked from the UNKNOWN set, saving N iterations

			// Construct the potential and the gradient
			// Note: This should get the goal cells from a specific group,
			//       but how will we select them?
			mGrid.UpdateGroupPotentialField(mGoals[objectGroupIt->first], objectGroupIt->second);

			// Update the object locations
			for (goalCellIt = objectGroupIt->second.begin(); goalCellIt != objectGroupIt->second.end(); ++goalCellIt) {
				mGrid.UpdateSimObjectLocation(*goalCellIt);
			}
		}

		// Enforce minimum distance between objects
		// Should this be handled in the EVENT_SIMOBJECT_COLLISION ?
	}

	#ifdef CCPATHMODULE_PROFILE
	printf("%s time: %ums\n", s.c_str(), (ScopedTimer::GetTaskTime(s) - t));;
	printf("\n");
	#endif
}

void PathModule::Kill() {
	std::cout << "[CCPathModule::Kill]" << std::endl;
}



void PathModule::AddObjectToGroup(unsigned int objID, unsigned int groupID) {
	if (mObjectGroups.find(groupID) == mObjectGroups.end()) {
		mObjectGroups[groupID] = std::set<unsigned int>();
	}

	if (mGoals.find(groupID) == mGoals.end()) {
		mGoals[groupID] = std::vector<Grid::Cell*>();
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
		}

		return true;
	}

	// object was not in a group
	return false;
}






unsigned int PathModule::GetScalarDataArraySizeX(unsigned int dataType) const {
	switch (dataType) {
		//! NOTE: these are not all the same size (eg. h vs. delta-h)!
		case DATATYPE_DENSITY: { return mGrid.GetGridWidth(); } break;
		//! case DATATYPE_DISCOMFORT: { return mGrid.GetSizeX(); } break;
		case DATATYPE_POTENTIAL: { return mGrid.GetGridWidth(); } break;
		case DATATYPE_HEIGHT: { return mGrid.GetGridWidth(); } break;
		default: {
		} break;
	}

	return 0;
}

unsigned int PathModule::GetScalarDataArraySizeZ(unsigned int dataType) const {
	switch (dataType) {
		//! NOTE: these are not all the same size (eg. h vs. delta-h)!
		case DATATYPE_DENSITY: { return mGrid.GetGridHeight(); } break;
		//! case DATATYPE_DISCOMFORT: { return mGrid.GetSizeZ(); } break;
		case DATATYPE_POTENTIAL: { return mGrid.GetGridHeight(); } break;
		case DATATYPE_HEIGHT: { return mGrid.GetGridHeight(); } break;
		default: {
		} break;
	}

	return 0;
}

const float* PathModule::GetScalarDataArray(unsigned int dataType, unsigned int groupID) const {
	switch (dataType) {
		case DATATYPE_DENSITY: {
			return mGrid.GetDensityDataArray();
		} break;
		case DATATYPE_DISCOMFORT: {
			//! TODO, FIXME: per-group
			//! return mGrid.GetDiscomfortDataArray();
		} break;
		case DATATYPE_POTENTIAL: {
			//! FIXME: per-group
			return mGrid.GetPotentialDataArray();
		} break;
		case DATATYPE_HEIGHT: {
			return mGrid.GetHeightDataArray();
		} break;
		default: {
		} break;
	}

	return NULL;
}



unsigned int PathModule::GetVectorDataArraySizeX(unsigned int dataType) const {
	switch (dataType) {
		//! NOTE: these are not all the same size (eg. v vs. v-bar)!
		//! case DATATYPE_COST: { return mGrid.GetSizeX(); } break;
		//! case DATATYPE_SPEED: { return mGrid.GetSizeX(); } break;
		//! case DATATYPE_POTENTIAL_DELTA: { return mGrid.GetSizeX(); } break;
		//! case DATATYPE_HEIGHT_DELTA: { return mGrid.GetSizeX(); } break;
		//! case DATATYPE_VELOCITY: { return mGrid.GetSizeX(); } break;
		//! case DATATYPE_VELOCITY_AVG: { return mGrid.GetSizeX(); } break;
		default: {
		} break;
	}

	return 0;
}

unsigned int PathModule::GetVectorDataArraySizeZ(unsigned int dataType) const {
	switch (dataType) {
		//! NOTE: these are not all the same size (eg. v vs. v-bar)!
		//! case DATATYPE_COST: { return mGrid.GetSizeZ(); } break;
		//! case DATATYPE_SPEED: { return mGrid.GetSizeZ(); } break;
		//! case DATATYPE_POTENTIAL_DELTA: { return mGrid.GetSizeZ(); } break;
		//! case DATATYPE_HEIGHT_DELTA: { return mGrid.GetSizeZ(); } break;
		//! case DATATYPE_VELOCITY: { return mGrid.GetSizeZ(); } break;
		//! case DATATYPE_VELOCITY_AVG: { return mGrid.GetSizeZ(); } break;
		default: {
		} break;
	}

	return 0;
}

const vec3f* PathModule::GetVectorDataArray(unsigned int dataType, unsigned int groupID) const {
	switch (dataType) {
		case DATATYPE_COST: {
			//! TODO, FIXME: per-group
			//! return mGrid.GetCostDataArray();
		} break;
		case DATATYPE_SPEED: {
			//! TODO, FIXME: per-group
			//! return mGrid.GetSpeedDataArray();
		} break;

		case DATATYPE_POTENTIAL_DELTA: {
			//! TODO, FIXME: per-group
			//! return mGrid.GetPotentialDeltaDataArray();
		} break;
		case DATATYPE_HEIGHT_DELTA: {
			//! TODO, NOTE: slopes are stored per-edge, four values per cell
			//! return mGrid.GetHeightDeltaDataArray();
		} break;

		case DATATYPE_VELOCITY: {
			//! TODO, FIXME: per-group
			//! return mGrid.GetVelocityDataArray();
		} break;
		case DATATYPE_VELOCITY_AVG: {
			//! TODO
			//! return mGrid.GetVelocityAvgDataArray();
		} break;
		default: {
		} break;
	}

	return NULL;
}
