#ifndef PFFG_PATH_MODULE_HDR
#define PFFG_PATH_MODULE_HDR

#include "./IPathModule.hpp"

class PathModule: public IPathModule {
public:
	PathModule(ICallOutHandler* icoh): IPathModule(icoh) {
	}

	void Init();
	void Update();
	void Kill();
};

IPathModule* CALL_CONV GetPathModuleInstance(ICallOutHandler* icoh) { return (new PathModule(icoh)); }
void         CALL_CONV FreePathModuleInstance(IPathModule* m) { delete ((PathModule*) m); }

#endif
