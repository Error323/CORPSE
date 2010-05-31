#ifndef PFFG_ICALLOUT_HANDLER_HDR
#define PFFG_ICALLOUT_HANDLER_HDR

#include "../Math/mat44fwd.hpp"
#include "../Math/vec3fwd.hpp"

// exposes simulation state to libraries
class ICallOutHandler {
public:
	virtual int GetHeightMapSizeX() const = 0;
	virtual int GetHeightMapSizeZ() const = 0;
	virtual float GetMinMapHeight() const = 0;
	virtual float GetMaxMapHeight() const = 0;
	virtual const float* GetCenterHeightMap() const = 0;
	virtual const float* GetCornerHeightMap() const = 0;
	virtual const float* GetSlopeMap() const = 0;

	virtual unsigned int GetMaxSimObjects() const = 0;
	virtual unsigned int GetNumSimObjects() const = 0;
	virtual unsigned int GetFreeSimObjectIDs(unsigned int* array, unsigned int size) const = 0;
	virtual unsigned int GetUsedSimObjectIDs(unsigned int* array, unsigned int size) const = 0;

	virtual const mat44f& GetSimObjectMatrix(unsigned int id) const = 0;
	virtual const vec3f& GetSimObjectPosition(unsigned int id) const = 0;
	virtual const vec3f& GetSimObjectDirection(unsigned int id) const = 0;

	virtual void SetSimObjectWantedPosition(unsigned int id, const vec3f& pos) const = 0;
	virtual void SetSimObjectWantedDirection(unsigned int id, const vec3f& dir) const = 0;
};

#endif
