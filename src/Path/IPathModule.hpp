#ifndef PFFG_IPATH_MODULE_HDR
#define PFFG_IPATH_MODULE_HDR

#include "../System/IEngineModule.hpp"

#include "../Math/mat44fwd.hpp"
#include "../Math/vec3fwd.hpp"

class IEvent;
class ICallOutHandler;
class IPathModule: public IEngineModule {
public:
	IPathModule(ICallOutHandler* icoh): coh(icoh) {}
	virtual ~IPathModule() {}

	virtual void OnEvent(const IEvent*) {}

	virtual void Init() {}
	virtual void Update() {}
	virtual void Kill() {}

	virtual ICallOutHandler* GetCallOutHandler() const { return coh; }

	virtual unsigned int GetNumGroupIDs() const = 0;
	virtual unsigned int GetGroupIDs(unsigned int* array, unsigned int size) const = 0;

	virtual unsigned int GetScalarDataArraySizeX(unsigned int dataType) const = 0;
	virtual unsigned int GetScalarDataArraySizeZ(unsigned int dataType) const = 0;
	virtual const float* GetScalarDataArray(unsigned int dataType, unsigned int groupID) const = 0;
	virtual unsigned int GetVectorDataArraySizeX(unsigned int dataType) const = 0;
	virtual unsigned int GetVectorDataArraySizeZ(unsigned int dataType) const = 0;
	virtual const vec3f* GetVectorDataArray(unsigned int dataType, unsigned int groupID) const = 0;

protected:
	ICallOutHandler* coh;
};

// engine gets and frees a module instance through these; implemented library-side
extern "C" SLIB_EXPORT   IPathModule*   CALL_CONV   GetPathModuleInstance(ICallOutHandler*);
extern "C" SLIB_EXPORT   void           CALL_CONV   FreePathModuleInstance(IPathModule*);

#endif
