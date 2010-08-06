#ifndef PFFG_SIMOBJECTDEF_HDR
#define PFFG_SIMOBJECTDEF_HDR

#include <string>

struct ModelBase;
class SimObjectDef {
public:
	SimObjectDef(unsigned int objID): id(objID) {
		model                = NULL;
		maxForwardSpeed      = 0.0f;
		maxTurningRate       = 0.0f;
		maxAccelerationRate  = 0.0f;
		maxDeccelerationRate = 0.0f;
		minSlopeAngleCosine  = 0.0f;
		maxSlopeAngleCosine  = 0.0f;
		minTerrainHeight     = 0.0f;
		maxTerrainHeight     = 0.0f;
		radius               = 1.0f;
	}

	unsigned int GetID() const { return id; }

	void SetModelName(const std::string& m) { modelName = m; }
	const std::string& GetModelName() const { return modelName; }
	void SetModel(ModelBase* mdl) { model = mdl; }
	ModelBase* GetModel() const { return model; }

	void SetMaxForwardSpeed(float f) { maxForwardSpeed = f; }
	void SetMaxTurningRate(float f) { maxTurningRate = f; }
	void SetMaxAccelerationRate(float f) { maxAccelerationRate = f; }
	void SetMaxDeccelerationRate(float f) { maxDeccelerationRate = f; }
	void SetMinSlopeAngleCosine(float f) { minSlopeAngleCosine = f; }
	void SetMaxSlopeAngleCosine(float f) { maxSlopeAngleCosine = f; }
	void SetMinTerrainHeight(float f) { minTerrainHeight = f; }
	void SetMaxTerrainHeight(float f) { maxTerrainHeight = f; }
	void SetRadius(float f) { radius = f; }

	float GetMaxForwardSpeed() const { return maxForwardSpeed; }
	float GetMaxTurningRate() const { return maxTurningRate; }
	float GetMaxAccelerationRate() const { return maxAccelerationRate; }
	float GetMaxDeccelerationRate() const { return maxDeccelerationRate; }
	float GetMinSlopeAngleCosine() const { return minSlopeAngleCosine; } 
	float GetMaxSlopeAngleCosine() const { return maxSlopeAngleCosine; } 
	float GetMinTerrainHeight() const { return minTerrainHeight; }
	float GetMaxTerrainHeight() const { return maxTerrainHeight; }
	float GetRadius() const { return radius; }

private:
	unsigned int id;

	std::string modelName;
	ModelBase* model;

	float maxForwardSpeed;       // must be specified in units per second, converted to units per frame
	float maxTurningRate;        // must be specified in degrees per second, converted to degrees per frame
	float maxAccelerationRate;   // must be specified in units per second^2, converted to units per frame
	float maxDeccelerationRate;  // must be specified in units per second^2, converted to units per frame
	float minSlopeAngleCosine;   // minimum slope (1.0 - terrain-normal y-component) this object can traverse
	float maxSlopeAngleCosine;   // maximum slope (1.0 - terrain-normal y-component) this object can traverse
	float minTerrainHeight;      // minimum terrain height this object can exist at
	float maxTerrainHeight;      // maximum terrain height this object can exist at
	float radius;                // In real world coords, should be a function of the unit's mass?
};

#endif
