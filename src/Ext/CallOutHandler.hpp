#ifndef PFFG_CALLOUT_HANDLER_HDR
#define PFFG_CALLOUT_HANDLER_HDR

#include "../Math/mat44fwd.hpp"
#include "../Math/vec3fwd.hpp"

class CallOutHandler {
public:
	static CallOutHandler* GetInstance();
	static void FreeInstance(CallOutHandler*);

	unsigned int GetMaxSimObjects() const;

	const mat44f& GetObjectMatrix(unsigned int id) const;
	const vec3f& GetObjectPosition(unsigned int id) const;
	const vec3f& GetObjectDirection(unsigned int id) const;
};

#endif
