#ifndef PFFG_SIMOBJECT_HDR
#define PFFG_SIMOBJECT_HDR

#include <string>

#include "../Math/mat44fwd.hpp"
#include "../Math/mat44.hpp"
#include "../Math/vec3fwd.hpp"
#include "../Math/vec3.hpp"

struct LocalModel;
class SimObjectDef;
class SimObject {
public:
	SimObject(SimObjectDef*, unsigned int);
	virtual ~SimObject();

	virtual void Update();

	const SimObjectDef* GetDef() const { return def; }
	unsigned int GetID() const { return id; }

	void SetModel(LocalModel* m) { mdl = m; }
	LocalModel* GetModel() const { return mdl; }

	const mat44f& GetMat() const { return physicalState.mat; }
	void SetMat(const mat44f& m) { physicalState.mat = m; }
	const vec3f& GetPos() const {  return (physicalState.mat).GetPos(); } // wrapper

	float GetCurrentForwardSpeed() const { return physicalState.currentForwardSpeed; }

	float GetWantedForwardSpeed() const { return physicalState.wantedForwardSpeed; }
	const vec3f& GetWantedPosition() const { return physicalState.wantedPos; }
	const vec3f& GetWantedDirection() const { return physicalState.wantedDir; }
	void SetWantedForwardSpeed(float f) { physicalState.wantedForwardSpeed = f; }
	void SetWantedPosition(const vec3f& pos) { physicalState.wantedPos = pos; }
	void SetWantedDirection(const vec3f& dir) { physicalState.wantedDir = dir; }

private:
	const SimObjectDef* def;
	const unsigned int id;

	LocalModel* mdl;

	struct PhysicalState {
		PhysicalState& operator = (const PhysicalState& state) {
			mat = state.mat;

			wantedPos = state.wantedPos;
			wantedDir = state.wantedDir;

			currentForwardSpeed = state.currentForwardSpeed;
			wantedForwardSpeed = state.wantedForwardSpeed;

			return *this;
		}

		PhysicalState& operator + (const PhysicalState& state) {
			mat.SetPos(mat.GetPos() + state.mat.GetPos());
			mat.SetXDir(mat.GetXDir() + state.mat.GetXDir());
			mat.SetYDir(mat.GetYDir() + state.mat.GetYDir());
			mat.SetZDir(mat.GetZDir() + state.mat.GetZDir());

			wantedPos += state.wantedPos;
			wantedDir += state.wantedDir;

			currentForwardSpeed += state.currentForwardSpeed;
			wantedForwardSpeed += state.wantedForwardSpeed;

			return *this;
		}
		PhysicalState& operator / (float s) {
			// note: re-orthonormalize?
			mat.SetPos(mat.GetPos() / s);
			mat.SetXDir((mat.GetXDir() / s).norm());
			mat.SetYDir((mat.GetYDir() / s).norm());
			mat.SetZDir((mat.GetZDir() / s).norm());

			wantedPos = (wantedPos / s);
			wantedDir = (wantedDir / s).norm();

			currentForwardSpeed /= s;
			wantedForwardSpeed /= s;

			return *this;
		}

		void Update(const SimObjectDef*);

		// world-space transform matrix
		mat44f mat;

		// where this object wants to be in world-space
		// (and in which direction it wants to be facing)
		vec3f wantedPos;
		vec3f wantedDir;

		// speed-scale this object is currently moving at
		float currentForwardSpeed;
		// speed-scale this object wants to be moving at
		float wantedForwardSpeed;
	};
	PhysicalState physicalState;
};

#endif
