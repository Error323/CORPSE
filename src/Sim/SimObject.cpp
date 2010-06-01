#include "./SimObject.hpp"
#include "./SimObjectDef.hpp"
#include "../Math/Trig.hpp"
#include "../Map/Ground.hpp"
#include "../Renderer/Models/ModelReaderBase.hpp"

SimObject::SimObject(SimObjectDef* d, unsigned int i): def(d), id(i) {
	currentForwardSpeed = 0.0f;
	wantedForwardSpeed = def->GetMaxForwardSpeed();
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
	vec3f forwardDir = mat.GetZDir();
		forwardDir.y = 0.0f;
		forwardDir.inorm();

	// figure out whether to turn left or right to match wantedDir
	//
	// the WORLD-space coordinate system is inverted along the Z-axis
	// (ie. has its origin in the top-left corner) with respect to the
	// mathematical definition (where the origin lies bottom-left); the
	// OBJECT-space coordinate system is inverted along the X-axis with
	// respect to the world-space coordinate system
	//
	// therefore, to get the proper angles wrt. the mathematical x-axis
	// that atan2 uses we need to flip the sign of both forwardDir.x AND
	// forwardDir.z
	//
	if (def->GetMaxTurningRate() != 0.0f) {
		float forwardGlobalAngleRad = atan2f(-forwardDir.z, -forwardDir.x);
		float wantedGlobalAngleRad = atan2f(-wantedDir.z, -wantedDir.x);
		float deltaGlobalAngleRad = 0.0f;

		if (forwardGlobalAngleRad < 0.0f) { forwardGlobalAngleRad += (M_PI * 2.0f); }
		if (wantedGlobalAngleRad < 0.0f) { wantedGlobalAngleRad += (M_PI * 2.0f); }

		deltaGlobalAngleRad = (forwardGlobalAngleRad - wantedGlobalAngleRad);

		// take the shorter of the two possible turns
		// (positive angle means a right turn and vv)
		if (deltaGlobalAngleRad >  M_PI) { deltaGlobalAngleRad = -((M_PI * 2.0f) - deltaGlobalAngleRad); }
		if (deltaGlobalAngleRad < -M_PI) { deltaGlobalAngleRad =  ((M_PI * 2.0f) + deltaGlobalAngleRad); }

		if (fabs(deltaGlobalAngleRad) > 0.01f) {
			mat.RotateY(DEG2RAD(def->GetMaxTurningRate()) * ((deltaGlobalAngleRad > 0.0f)? 1.0f: -1.0f));
		}
	}


	wantedForwardSpeed = std::max(0.0f, std::min(wantedForwardSpeed, def->GetMaxForwardSpeed()));

	if (currentForwardSpeed <= wantedForwardSpeed) {
		// accelerate (at maximum acceleration-rate) to match wantedSpeed
		currentForwardSpeed += def->GetMaxAccelerationRate();
		currentForwardSpeed = std::min(currentForwardSpeed, wantedForwardSpeed);
	} else {
		// deccelerate (at maximum decceleration-rate) to match wantedSpeed
		currentForwardSpeed -= def->GetMaxDeccelerationRate();
		currentForwardSpeed = std::max(currentForwardSpeed, 0.0f);
	}

	// note: no gravity, since we only simulate earth-bound objects
	vec3f pos = mat.GetPos() + (mat.GetZDir() * currentForwardSpeed);
		pos.y = ground->GetHeight(pos.x, pos.z);

	mat.SetPos(pos);
	mat.SetYDirXZ(ground->GetSmoothNormal(pos.x, pos.z));
}
