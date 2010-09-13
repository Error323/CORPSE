#include "./SimObjectState.hpp"
#include "./SimObject.hpp"
#include "./SimObjectDef.hpp"
#include "../Math/Trig.hpp"
#include "../Map/Ground.hpp"

void PhysicalState::Update(const SimObject* owner) {
	if (!enabled || (owner->GetWantedPhysicalStates()).empty()) {
		return;
	}

	const SimObjectDef* def = owner->GetDef();
	const WantedPhysicalState& wps = owner->GetWantedPhysicalState(true);
	const vec3f& wantedDir = wps.wantedDir;

	vec3f currentPos = mat.GetPos();
	vec3f forwardDir = mat.GetZDir();
		forwardDir.y = 0.0f;
		forwardDir.inorm2D();

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

		const bool  instaTurn = (std::fabs(deltaGlobalAngleRad) <= DEG2RAD(def->GetMaxTurningRate()));
		const float turnAngle = instaTurn? deltaGlobalAngleRad: DEG2RAD(def->GetMaxTurningRate());

		if (std::fabs(turnAngle) > DEG2RAD(1.0f)) {
			mat.RotateY(turnAngle * ((deltaGlobalAngleRad > 0.0f)? 1.0f: -1.0f));
		}
	}

	{
		// slope lies in [0=horizontal, 1=vertical],
		// so we have no information about the sign
		const float zdiry = (mat.GetZDir()).y;
		const float slope = ground->GetSlope(currentPos.x, currentPos.z);

		// slow down on positive slopes, speed up on negative slopes
		//    positive slope and forward  motion ==> slow down to lower  positive speed
		//    positive slope and backward motion ==> speed up  to higher negative speed
		//    negative slope and forward  motion ==> speed up  to higher positive speed
		//    negative slope and backward motion ==> slow down to lower  negative speed
		const bool forwardMotion = (speed >  0.05f);
		const bool bckwardMotion = (speed < -0.05f);
		const bool positiveSlope = (zdiry >  0.05f);
		const bool negativeSlope = (zdiry < -0.05f);

		const bool allowSlopeAcc =
			(forwardMotion && negativeSlope && (speed < def->GetMaxForwardSpeed() * 1.75f)) ||
			(bckwardMotion && positiveSlope && (speed < def->GetMaxForwardSpeed() * 1.75f));
		const bool allowSlopeDec =
			(forwardMotion && positiveSlope && (speed > def->GetMaxForwardSpeed() * 0.25f)) ||
			(bckwardMotion && negativeSlope && (speed > def->GetMaxForwardSpeed() * 0.25f));

		if (allowSlopeAcc) { speed += (speed * slope * (forwardMotion?  1.0f: -1.0f)); }
		if (allowSlopeDec) { speed -= (speed * slope * (bckwardMotion? -1.0f:  1.0f)); }
	}

	if (wps.wantedSpeed >= 0.0f) {
		if (speed <= wps.wantedSpeed) {
			// accelerate (at maximum acceleration-rate) to match wantedSpeed
			speed += def->GetMaxAccelerationRate();
			speed = std::min(speed, wps.wantedSpeed);
		} else {
			// deccelerate (at maximum decceleration-rate) to match wantedSpeed
			speed -= def->GetMaxDeccelerationRate();
			speed = std::max(speed, 0.0f);
		}
	} else {
		if (speed >= wps.wantedSpeed) {
			speed -= def->GetMaxAccelerationRate();
			speed = std::max(speed, wps.wantedSpeed);
		} else {
			speed += def->GetMaxDeccelerationRate();
			speed = std::min(speed, 0.0f);
		}
	}

	// note: no gravity, since we only simulate earth-bound objects
	vec3f pos = mat.GetPos() + (mat.GetZDir() * speed);
		pos.y = ground->GetHeight(pos.x, pos.z);

	mat.SetPos(pos);
	mat.SetYDirXZ(ground->GetSmoothNormal(pos.x, pos.z));
}
