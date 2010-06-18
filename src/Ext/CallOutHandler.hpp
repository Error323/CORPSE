#ifndef PFFG_CALLOUT_HANDLER_HDR
#define PFFG_CALLOUT_HANDLER_HDR

#include "./ICallOutHandler.hpp"

#include "../Math/mat44fwd.hpp"
#include "../Math/vec3fwd.hpp"

class CallOutHandler: public ICallOutHandler {
public:
	static CallOutHandler* GetInstance();
	static void FreeInstance(CallOutHandler*);

	int GetHeightMapSizeX() const;
	int GetHeightMapSizeZ() const;
	float GetMinMapHeight() const;
	float GetMaxMapHeight() const;
	int GetSquareSize() const;
	const float* GetCenterHeightMap() const;
	const float* GetCornerHeightMap() const;
	const float* GetSlopeMap() const;

	unsigned int GetNumSimObjectDefs() const;
	const SimObjectDef* GetRawSimObjectDef(unsigned int defID) const;

	unsigned int GetMaxSimObjects() const;
	unsigned int GetNumSimObjects() const;

	unsigned int GetObjectIDs(const vec3f&, const vec3f&, unsigned int*, unsigned int) const;
	unsigned int GetClosestObjectID(const vec3f&, float) const;

	unsigned int GetFreeSimObjectIDs(unsigned int*, unsigned int) const;
	unsigned int GetUsedSimObjectIDs(unsigned int*, unsigned int) const;

	bool IsValidSimObjectID(unsigned int) const;

	const SimObjectDef* GetSimObjectDef(unsigned int) const;
	const mat44f& GetSimObjectMatrix(unsigned int) const;
	const vec3f& GetSimObjectPosition(unsigned int) const;
	const vec3f& GetSimObjectDirection(unsigned int) const;
	float GetSimObjectCurrentForwardSpeed(unsigned int) const;

	float GetSimObjectWantedForwardSpeed(unsigned int) const;
	const vec3f& GetSimObjectWantedPosition(unsigned int) const;
	const vec3f& GetSimObjectWantedDirection(unsigned int) const;
	void SetSimObjectWantedForwardSpeed(unsigned int, float) const;
	void SetSimObjectWantedPosition(unsigned int, const vec3f&) const;
	void SetSimObjectWantedDirection(unsigned int, const vec3f&) const;
};

#endif
