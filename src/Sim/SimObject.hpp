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

	const mat44f& GetMat() const { return mat; }
	void SetMat(const mat44f& m) { mat = m; }

	float GetCurrentForwardSpeed() const { return currentForwardSpeed; }

	void SetWantedForwardSpeed(float f) { wantedForwardSpeed = f; }
	void SetWantedPosition(const vec3f& pos) { wantedPos = pos; }
	void SetWantedDirection(const vec3f& dir) { wantedDir = dir; }

private:
	const SimObjectDef* def;
	const unsigned int id;

	LocalModel* mdl;

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

#endif
