#ifndef PFFG_ICALLOUT_HANDLER_HDR
#define PFFG_ICALLOUT_HANDLER_HDR

#include "../Math/mat44fwd.hpp"
#include "../Math/vec3fwd.hpp"

// exposes engine state to libraries
class ICallOutHandler {
public:
	virtual unsigned int GetMaxSimObjects() const = 0;

	virtual const mat44f& GetObjectMatrix(unsigned int id) const = 0;
	virtual const vec3f& GetObjectPosition(unsigned int id) const = 0;
	virtual const vec3f& GetObjectDirection(unsigned int id) const = 0;

	virtual void SetObjectMatrix(unsigned int id, const mat44f&) const = 0;
	virtual void SetObjectPosition(unsigned int id, const vec3f&) const = 0;
	virtual void SetObjectDirection(unsigned int id, const vec3f&) const = 0;
};

#endif
