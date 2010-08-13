#ifndef PFFG_SIMOBJECTSTATE_HDR
#define PFFG_SIMOBJECTSTATE_HDR

#include "../Math/mat44fwd.hpp"
#include "../Math/mat44.hpp"
#include "../Math/vec3fwd.hpp"
#include "../Math/vec3.hpp"

class SimObject;
struct PhysicalState {
	PhysicalState(): currentForwardSpeed(0.0f), moved(false) {
	}

	PhysicalState& operator = (const PhysicalState& state) {
		moved = (state.mat.GetPos() != mat.GetPos());
		mat = state.mat; currentForwardSpeed = state.currentForwardSpeed;
		return *this;
	}

	PhysicalState& operator + (const PhysicalState& state) {
		mat.SetPos(mat.GetPos() + state.mat.GetPos());
		mat.SetXDir(mat.GetXDir() + state.mat.GetXDir());
		mat.SetYDir(mat.GetYDir() + state.mat.GetYDir());
		mat.SetZDir(mat.GetZDir() + state.mat.GetZDir());

		currentForwardSpeed += state.currentForwardSpeed;
		return *this;
	}
	PhysicalState& operator / (float s) {
		// note: re-orthonormalize?
		mat.SetPos(mat.GetPos() / s);
		mat.SetXDir((mat.GetXDir() / s).norm());
		mat.SetYDir((mat.GetYDir() / s).norm());
		mat.SetZDir((mat.GetZDir() / s).norm());

		currentForwardSpeed /= s;
		return *this;
	}

	void Update(const SimObject*);

	// world-space transform matrix
	mat44f mat;

	// speed-scale this object is currently moving at
	float currentForwardSpeed;
	bool moved;
};

struct WantedPhysicalState {
	WantedPhysicalState(): wantedForwardSpeed(0.0f) {
	}

	// where this object wants to be in world-space
	// (and in which direction it wants to be facing)
	vec3f wantedPos;
	vec3f wantedDir;

	// speed-scale this object wants to be moving at
	float wantedForwardSpeed;
};

#endif
