#ifndef PFFG_IPATH_MODULE_HDR
#define PFFG_IPATH_MODULE_HDR

#include "../System/IEngineModule.hpp"

#include "../Math/mat44fwd.hpp"
#include "../Math/vec3fwd.hpp"

class IPathModule: public IEngineModule {
public:
	virtual ~IPathModule() {}

	virtual void Init() {}
	virtual void Update() {}
	virtual void Kill() {}
};

extern "C" SLIB_EXPORT   IPathModule*   CALL_CONV   GetPathModuleInstance();
extern "C" SLIB_EXPORT   void           CALL_CONV   FreePathModuleInstance(IPathModule*);

#endif
