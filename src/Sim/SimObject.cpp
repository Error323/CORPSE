#include "./SimObject.hpp"
#include "./SimObjectDef.hpp"
#include "../Map/Ground.hpp"
#include "../Renderer/Models/ModelReaderBase.hpp"

SimObject::SimObject(SimObjectDef* d, unsigned int i): def(d), id(i) {
	currentSpeed = 0.0f;
	wantedSpeed = def->GetMaxForwardSpeed();
}

SimObject::~SimObject() {
	// need to clean this up elsewhere
	// (possibly have the renderer wait
	// for SimObjectDestroyed events?)
	delete mdl;

	mdl = 0;
	def = 0;
}

void SimObject::Update() {
	const vec3f& zdir = mat.GetZDir();

	if (zdir.dot3D(wantedDir) < 1.0f) {
		// TODO: turn (at maximum turning-rate) to match wantedDir
	}

	if (currentSpeed < wantedSpeed) {
		// accelerate (at maximum acceleration-rate) to match wantedSpeed
		currentSpeed += def->GetMaxAccelerationRate();
		currentSpeed = std::min(currentSpeed, wantedSpeed);
	} else {
		// deccelerate (at maximum decceleration-rate) to match wantedSpeed
		currentSpeed -= def->GetMaxDeccelerationRate();
		currentSpeed = std::max(currentSpeed, 0.0f);
	}

	vec3f pos = mat.GetPos() + (mat.GetZDir() * currentSpeed);
		pos.y = std::min(pos.y, ground->GetHeight(pos.x, pos.z));

	mat.SetPos(pos);
	mat.SetZDir(zdir);
	mat.SetYDirXZ(ground->GetNormal(pos.x, pos.z));
}
