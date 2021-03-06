#ifndef PFFG_IENGINE_MODULE_HDR
#define PFFG_IENGINE_MODULE_HDR

#define SLIB_EXPORT __attribute__((visibility("default")))
#define CALL_CONV   __attribute__((stdcall))

#include "../System/IEventReceiver.hpp"

class IEngineModule: public IEventReceiver {
public:
	virtual ~IEngineModule() {}

	virtual void Init() = 0;
	virtual void Update() = 0;
	virtual void Kill() = 0;
};

#endif
