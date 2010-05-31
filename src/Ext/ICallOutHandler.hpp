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

	virtual const mat44f& GetObjectMatrix(unsigned int id) const = 0;
	virtual const vec3f& GetObjectPosition(unsigned int id) const = 0;
	virtual const vec3f& GetObjectDirection(unsigned int id) const = 0;

	virtual void SetObjectMatrix(unsigned int id, const mat44f&) const = 0;
	virtual void SetObjectPosition(unsigned int id, const vec3f&) const = 0;
	virtual void SetObjectDirection(unsigned int id, const vec3f&) const = 0;
};

#endif
