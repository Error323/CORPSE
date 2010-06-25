#ifndef PFFG_ICALLOUT_HANDLER_HDR
#define PFFG_ICALLOUT_HANDLER_HDR

#include "../Math/mat44fwd.hpp"
#include "../Math/vec3fwd.hpp"

class SimObjectDef;
struct WantedPhysicalState;

// exposes simulation state to libraries
class ICallOutHandler {
public:
	virtual int GetHeightMapSizeX() const = 0;
	virtual int GetHeightMapSizeZ() const = 0;
	virtual float GetMinMapHeight() const = 0;
	virtual float GetMaxMapHeight() const = 0;
	virtual int GetSquareSize() const = 0;
	virtual const float* GetCenterHeightMap() const = 0;
	virtual const float* GetCornerHeightMap() const = 0;
	virtual const float* GetSlopeMap() const = 0;

	virtual unsigned int GetNumSimObjectDefs() const = 0;
	virtual const SimObjectDef* GetRawSimObjectDef(unsigned int defID) const = 0;

	virtual unsigned int GetMaxSimObjects() const = 0;
	virtual unsigned int GetNumSimObjects() const = 0;

	virtual unsigned int GetObjectIDs(const vec3f& pos, const vec3f& radii, unsigned int* array, unsigned int size) const = 0;
	virtual unsigned int GetClosestObjectID(const vec3f& pos, float radius) const = 0;

	virtual unsigned int GetFreeSimObjectIDs(unsigned int* array, unsigned int size) const = 0;
	virtual unsigned int GetUsedSimObjectIDs(unsigned int* array, unsigned int size) const = 0;

	virtual bool IsValidSimObjectID(unsigned int objID) const = 0;

	virtual const SimObjectDef* GetSimObjectDef(unsigned int objID) const = 0;
	virtual const mat44f& GetSimObjectMatrix(unsigned int objID) const = 0;
	virtual const vec3f& GetSimObjectPosition(unsigned int objID) const = 0;
	virtual const vec3f& GetSimObjectDirection(unsigned int objID) const = 0;
	virtual float GetSimObjectCurrentForwardSpeed(unsigned int objID) const = 0;

	virtual unsigned int GetSimObjectNumWantedPhysicalStates(unsigned int objID) const = 0;
	virtual bool PopSimObjectWantedPhysicalStates(unsigned int objID, unsigned int numStates) const = 0;
	virtual const WantedPhysicalState& GetSimObjectWantedPhysicalState(unsigned int objID) const = 0;
	virtual void SetSimObjectWantedPhysicalState(unsigned int objID, const WantedPhysicalState& state, bool queued) const = 0;
};

#endif
