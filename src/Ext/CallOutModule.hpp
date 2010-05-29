#ifndef PFFG_MODULE_CALLOUT_HANDLER_HDR
#define PFFG_MODULE_CALLOUT_HANDLER_HDR

#include "./ICallOutModule.hpp"

#include "../Math/mat44fwd.hpp"
#include "../Math/vec3fwd.hpp"

class CallOutModule: public ICallOutModule {
public:
	void Init() {}
	void Update() {}
	void Kill() {}

	const mat44f& GetObjectMatrix(unsigned int id) const;
	const vec3f& GetObjectPosition(unsigned int id) const;
	const vec3f& GetObjectDirection(unsigned int id) const;
};



ICallOutModule* CALL_CONV GetInstance(                  ) { return (new CallOutModule()); }
void            CALL_CONV FreeInstance(ICallOutModule* m) { delete ((CallOutModule*) m); }

#endif
