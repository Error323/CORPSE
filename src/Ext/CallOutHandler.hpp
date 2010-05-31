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
	const float* GetCenterHeightMap() const;
	const float* GetCornerHeightMap() const;
	const float* GetSlopeMap() const;

	unsigned int GetMaxSimObjects() const;
	unsigned int GetFreeSimObjectIDs(unsigned int* array, unsigned int size) const;
	unsigned int GetUsedSimObjectIDs(unsigned int* array, unsigned int size) const;

	const mat44f& GetObjectMatrix(unsigned int id) const;
	const vec3f& GetObjectPosition(unsigned int id) const;
	const vec3f& GetObjectDirection(unsigned int id) const;

	void SetObjectMatrix(unsigned int id, const mat44f&) const;
	void SetObjectPosition(unsigned int id, const vec3f&) const;
	void SetObjectDirection(unsigned int id, const vec3f&) const;
};

#endif
