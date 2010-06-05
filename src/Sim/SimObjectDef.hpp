#ifndef PFFG_SIMOBJECTDEF_HDR
#define PFFG_SIMOBJECTDEF_HDR

#include <string>

class SimObjectDef {
public:
	SimObjectDef(unsigned int objID): id(objID) {
		maxForwardSpeed      = 0.0f;
		maxTurningRate       = 0.0f;
		maxAccelerationRate  = 0.0f;
		maxDeccelerationRate = 0.0f;
		maxSlopeAngleCosine  = 0.0f;
	}

	unsigned int GetID() const { return id; }

	void SetModelName(const std::string& m) { modelName = m; }
	const std::string& GetModelName() const { return modelName; }

	void SetMaxForwardSpeed(float f) { maxForwardSpeed = f; }
	void SetMaxTurningRate(float f) { maxTurningRate = f; }
	void SetMaxAccelerationRate(float f) { maxAccelerationRate = f; }
	void SetMaxDeccelerationRate(float f) { maxDeccelerationRate = f; }
	void SetMaxSlopeAngleCosine(float f) { maxSlopeAngleCosine = f; }
	float GetMaxForwardSpeed() const { return maxForwardSpeed; }
	float GetMaxTurningRate() const { return maxTurningRate; }
	float GetMaxAccelerationRate() const { return maxAccelerationRate; }
	float GetMaxDeccelerationRate() const { return maxDeccelerationRate; }
	float GetMaxSlopeAngleCosine() const { return maxSlopeAngleCosine; } 

private:
	unsigned int id;

	std::string modelName;

	float maxForwardSpeed;       // must be specified in units per second, converted to units per frame
	float maxTurningRate;        // must be specified in degrees per second, converted to degrees per frame
	float maxAccelerationRate;   // must be specified in units per second^2, converted to units per frame
	float maxDeccelerationRate;  // must be specified in units per second^2, converted to units per frame
	float maxSlopeAngleCosine;   // maximum slope (1.0 - terrain-normal y-component) this object can traverse
};

#endif
