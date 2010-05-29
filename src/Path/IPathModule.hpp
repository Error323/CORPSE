#ifndef PFFG_IPATH_MODULE_HDR
#define PFFG_IPATH_MODULE_HDR

#include "../System/IEngineModule.hpp"

#include "../Math/mat44fwd.hpp"
#include "../Math/vec3fwd.hpp"

class ICallOutHandler;
class IPathModule: public IEngineModule {
public:
	IPathModule(ICallOutHandler* icoh): coh(icoh) {}
	virtual ~IPathModule() {}

	virtual void Init() {}
	virtual void Update() {}
	virtual void Kill() {}

	virtual ICallOutHandler* GetCallOutHandler() const { return coh; }

protected:
	ICallOutHandler* coh;
};

// engine gets and frees a module instance through these; implemented library-side
extern "C" SLIB_EXPORT   IPathModule*   CALL_CONV   GetPathModuleInstance(ICallOutHandler*);
extern "C" SLIB_EXPORT   void           CALL_CONV   FreePathModuleInstance(IPathModule*);

#endif
