#ifndef PFFG_IMODULE_CALLOUT_HANDLER_HDR
#define PFFG_IMODULE_CALLOUT_HANDLER_HDR

#include "../System/IEngineModule.hpp"

#include "../Math/mat44fwd.hpp"
#include "../Math/vec3fwd.hpp"

class ICallOutModule: public IEngineModule {
public:
	virtual const mat44f& GetObjectMatrix(unsigned int id) const = 0;
	virtual const vec3f& GetObjectPosition(unsigned int id) const = 0;
	virtual const vec3f& GetObjectDirection(unsigned int id) const = 0;
};

extern "C" SLIB_EXPORT   ICallOutModule*   CALL_CONV   GetInstance();
extern "C" SLIB_EXPORT   void              CALL_CONV   FreeInstance(ICallOutModule*);

#endif
