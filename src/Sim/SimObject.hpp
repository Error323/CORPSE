#ifndef PFFG_SIMOBJECT_HDR
#define PFFG_SIMOBJECT_HDR

#include <list>

#include "./SimObjectState.hpp"

struct LocalModel;
class SimObjectDef;
class SimObject {
public:
	SimObject(SimObjectDef* d, unsigned int oid, unsigned int tid): def(d), objectID(oid), teamID(tid) {
		// NOTE:
		//    this is set by rendering code when it receives the
		//    SimObjectCreatedEvent, so simulation logic will not
		//    see the proper value via GetModelRadius() until the
		//    next sim-frame
		mdlRadius = 0.0f;
	}

	virtual ~SimObject() {
		def = NULL;
		mdl = NULL;
	}

	virtual void Update();

	const SimObjectDef* GetDef() const { return def; }
	unsigned int GetID() const { return objectID; }
	unsigned int GetTeamID() const { return teamID; }

	void SetModel(LocalModel* m) { mdl = m; }
	LocalModel* GetModel() const { return mdl; }

	float GetModelRadius() const { return mdlRadius; }
	void SetModelRadius(float f) { mdlRadius = f; }

	const mat44f& GetMat() const { return physicalState.mat; }
	void SetMat(const mat44f& m) { physicalState.mat = m; }
	const vec3f& GetPos() const { return (physicalState.mat).GetPos(); } // wrapper
	bool HasMoved() const { return physicalState.moved; }

	// get or set this object's current physical state
	const PhysicalState& GetPhysicalState() const { return physicalState; }
	void SetPhysicalState(const PhysicalState& s) { physicalState = s; }

	const WantedPhysicalState& GetWantedPhysicalState(bool) const;
	void PushWantedPhysicalState(const WantedPhysicalState&, bool, bool);
	bool PopWantedPhysicalStates(unsigned int, bool);

	const std::list<WantedPhysicalState>& GetWantedPhysicalStates() const { return wantedPhysicalStates; }
	const std::list<PhysicalState>& GetPrevPhysicalStates() const { return prevPhysicalStates; }

private:
	const SimObjectDef* def;

	const unsigned int objectID;
	      unsigned int teamID;

	float mdlRadius;

	LocalModel* mdl;
	PhysicalState physicalState;

	std::list<WantedPhysicalState> wantedPhysicalStates;
	std::list<PhysicalState> prevPhysicalStates;
};

#endif
