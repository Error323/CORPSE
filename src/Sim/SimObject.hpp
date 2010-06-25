#ifndef PFFG_SIMOBJECT_HDR
#define PFFG_SIMOBJECT_HDR

#include <list>

#include "./SimObjectState.hpp"

struct LocalModel;
class SimObjectDef;
class SimObject {
public:
	SimObject(SimObjectDef* d, unsigned int _id): def(d), id(_id) {
	}

	virtual ~SimObject() {
		def = NULL;
		mdl = NULL;
	}

	virtual void Update();

	const SimObjectDef* GetDef() const { return def; }
	unsigned int GetID() const { return id; }

	void SetModel(LocalModel* m) { mdl = m; }
	LocalModel* GetModel() const { return mdl; }

	void SetRadius(float f) { mdlRadius = f; }
	float GetRadius() const { return mdlRadius; }

	const mat44f& GetMat() const { return physicalState.mat; }
	void SetMat(const mat44f& m) { physicalState.mat = m; }
	const vec3f& GetPos() const {  return (physicalState.mat).GetPos(); } // wrapper


	const PhysicalState& GetPhysicalState() const { return physicalState; }
	void SetPhysicalState(const PhysicalState& s) { physicalState = s; }

	const WantedPhysicalState& GetWantedPhysicalState() const {
		static WantedPhysicalState wps; // dummy

		if (!wantedPhysicalStates.empty()) {
			return wantedPhysicalStates.front();
		}

		return wps;
	}
	void PushWantedPhysicalState(const WantedPhysicalState& wps, bool queued) {
		if (!queued) {
			wantedPhysicalStates.clear();
		}

		wantedPhysicalStates.push_back(wps);
	}
	bool PopWantedPhysicalStates(unsigned int numStates) {
		if (wantedPhysicalStates.size() < numStates) {
			return false;
		}

		for (unsigned int n = 0; n < numStates; n++) {
			wantedPhysicalStates.pop_front();
		}

		return true;
	}

	const std::list<WantedPhysicalState>& GetWantedPhysicalStates() const { return wantedPhysicalStates; }

private:
	const SimObjectDef* def;
	const unsigned int id;

	LocalModel* mdl;
	float mdlRadius;

	PhysicalState physicalState;
	std::list<WantedPhysicalState> wantedPhysicalStates;
};

#endif
