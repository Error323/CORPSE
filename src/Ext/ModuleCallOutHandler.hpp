#ifndef PFFG_MODULE_CALLOUT_HANDLER_HDR
#define PFFG_MODULE_CALLOUT_HANDLER_HDR

#include "../Math/mat44fwd.hpp"
#include "../Math/vec3fwd.hpp"

class ModuleCallOutHandler {
public:
	static ModuleCallOutHandler* GetInstance();
	static void FreeInstance(ModuleCallOutHandler*);

	const vec3f& GetObjectPosition(unsigned int id) const;
	const vec3f& GetObjectDirection(unsigned int id) const;
};

#endif
