#ifndef PFFG_SIMOBJECTDEF_HDR
#define PFFG_SIMOBJECTDEF_HDR

#include <string>

class SimObjectDef {
public:
	SimObjectDef() {
		maxForwardSpeed      = 0.0f;
		maxTurningRate       = 0.0f;
		maxAccelerationRate  = 0.0f;
		maxDeccelerationRate = 0.0f;
	}

	void SetModelName(const std::string& m) { modelName = m; }
	const std::string& GetModelName() const { return modelName; }

	void SetMaxForwardSpeed(float f) { maxForwardSpeed = f; }
	void SetMaxTurningRate(float f) { maxTurningRate = f; }
	void SetMaxAccelerationRate(float f) { maxAccelerationRate = f; }
	void SetMaxDeccelerationRate(float f) { maxDeccelerationRate = f; }
	float GetMaxForwardSpeed() const { return maxForwardSpeed; }
	float GetMaxTurningRate() const { return maxTurningRate; }
	float GetMaxAccelerationRate() const { return maxAccelerationRate; }
	float GetMaxDeccelerationRate() const { return maxDeccelerationRate; }

private:
	std::string modelName;

	float maxForwardSpeed;
	float maxTurningRate;
	float maxAccelerationRate;
	float maxDeccelerationRate;
};

#endif
