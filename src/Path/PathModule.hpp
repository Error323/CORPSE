#ifndef PFFG_PATH_MODULE_HDR
#define PFFG_PATH_MODULE_HDR

#include "./IPathModule.hpp"

class CallOutHandler;
class PathModule: public IPathModule {
public:
	void Init();
	void Update();
	void Kill();

private:
	CallOutHandler* coh;
};



IPathModule* CALL_CONV GetPathModuleInstance(               ) { return (new PathModule()); }
void         CALL_CONV FreePathModuleInstance(IPathModule* m) { delete ((PathModule*) m); }

#endif
