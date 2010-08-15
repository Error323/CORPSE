#ifndef PFFG_SIMOBJECTSTATE_HDR
#define PFFG_SIMOBJECTSTATE_HDR

#include "../Math/mat44fwd.hpp"
#include "../Math/mat44.hpp"
#include "../Math/vec3fwd.hpp"
#include "../Math/vec3.hpp"

class SimObject;
struct PhysicalState {
	PhysicalState(): enabled(true), moved(false) {
		speed = 0.0f;
	}

	PhysicalState& operator = (const PhysicalState& state) {
		moved = (state.mat.GetPos() != mat.GetPos());
		enabled = state.enabled;

		mat = state.mat;
		speed = state.speed;
		return *this;
	}

	PhysicalState& operator + (const PhysicalState& state) {
		mat.SetPos(mat.GetPos() + state.mat.GetPos());
		mat.SetXDir(mat.GetXDir() + state.mat.GetXDir());
		mat.SetYDir(mat.GetYDir() + state.mat.GetYDir());
		mat.SetZDir(mat.GetZDir() + state.mat.GetZDir());

		speed += state.speed;
		return *this;
	}
	PhysicalState& operator / (float s) {
		// note: re-orthonormalize?
		mat.SetPos(mat.GetPos() / s);
		mat.SetXDir((mat.GetXDir() / s).norm());
		mat.SetYDir((mat.GetYDir() / s).norm());
		mat.SetZDir((mat.GetZDir() / s).norm());

		speed /= s;
		return *this;
	}

	void Update(const SimObject*);

	// world-space transform matrix
	mat44f mat;

	// speed-scale this object is currently moving at
	float speed;

	bool enabled;
	bool moved;
};

struct WantedPhysicalState {
	WantedPhysicalState(): wantedSpeed(0.0f) {
	}

	// where this object wants to be in world-space
	// (and in which direction it wants to be facing)
	vec3f wantedPos;
	vec3f wantedDir;

	// speed-scale this object wants to be moving at
	// if negative, the object wants to go backwards
	float wantedSpeed;
};

#endif
