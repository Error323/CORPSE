#ifndef PFFG_SIMOBJECT_HDR
#define PFFG_SIMOBJECT_HDR

#include <list>

#include "./SimObjectState.hpp"

struct LocalModel;
class SimObjectDef;
class SimObject {
public:
	SimObject(SimObjectDef* d, unsigned int _id): def(d), id(_id) {
		// NOTE:
		//    this is set by rendering code when it receives the
		//    SimObjectCreatedEvent, so simulation logic will not
		//    see the proper value via GetRadius() until the next
		//    sim-frame
		mdlRadius = 0.0f;
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
	const vec3f& GetPos() const { return (physicalState.mat).GetPos(); } // wrapper
	bool HasMoved() const { return physicalState.moved; }

	// get or set this object's current physical state
	const PhysicalState& GetPhysicalState() const { return physicalState; }
	void SetPhysicalState(const PhysicalState& s) { physicalState = s; }

	const WantedPhysicalState& GetWantedPhysicalState(bool) const;
	void PushWantedPhysicalState(const WantedPhysicalState&, bool, bool);
	bool PopWantedPhysicalStates(unsigned int, bool);

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
