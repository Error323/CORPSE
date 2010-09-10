#include "./SimObject.hpp"
#include "./SimObjectDef.hpp"

#define SIMOBJECT_TRACE_FRAMES 150

void SimObject::Update() {
	PhysicalState p = physicalState;

	#if (SIMOBJECT_TRACE_FRAMES > 0)
	prevPhysicalStates.push_front(p);

	if (prevPhysicalStates.size() >= SIMOBJECT_TRACE_FRAMES) {
		prevPhysicalStates.pop_back();
	}
	#endif

	p.Update(this);
	this->SetPhysicalState(p);
}



// get the first- or last-in-queue wanted state this object wants to be in
const WantedPhysicalState& SimObject::GetWantedPhysicalState(bool front) const {
	static WantedPhysicalState wps; // dummy

	if (!wantedPhysicalStates.empty()) {
		if (front) {
			return wantedPhysicalStates.front();
		} else {
			return wantedPhysicalStates.back();
		}
	}

	return wps;
}

// add a wanted state to the front or back of this object's queue
void SimObject::PushWantedPhysicalState(const WantedPhysicalState& wps, bool queued, bool front) {
	if (!queued) {
		wantedPhysicalStates.clear();
	}

	if (front) {
		wantedPhysicalStates.push_front(wps);
	} else {
		wantedPhysicalStates.push_back(wps);
	}
}

// remove a wanted state from the front or back of this object's queue
bool SimObject::PopWantedPhysicalStates(unsigned int numStates, bool front) {
	if (wantedPhysicalStates.size() < numStates) {
		return false;
	}

	for (unsigned int n = 0; n < numStates; n++) {
		if (front) {
			wantedPhysicalStates.pop_front();
		} else {
			wantedPhysicalStates.pop_back();
		}
	}

	return true;
}
