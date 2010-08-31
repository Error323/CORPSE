#ifndef PFFG_IPATH_MODULE_HDR
#define PFFG_IPATH_MODULE_HDR

#include "../System/IEngineModule.hpp"

#include "../Math/mat44fwd.hpp"
#include "../Math/vec3fwd.hpp"

#define DATATYPEINFO_CACHED {0, 0, 0, 0, 0, {NULL}, "", false, true};
#define DATATYPEINFO_RWRITE {0, 0, 0, 0, 0, {NULL}, "", false, false};

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

	struct DataTypeInfo {
		unsigned int type;
		unsigned int group;
		unsigned int sizex;
		unsigned int sizey;
		unsigned int stride;
		union {
			const float* fdata;
			const vec3f* vdata;
		};
		const char* name;
		bool global;
		bool cached;
	};

	virtual bool GetScalarDataTypeInfo(DataTypeInfo*) const = 0;
	virtual bool GetVectorDataTypeInfo(DataTypeInfo*) const = 0;
	virtual unsigned int GetNumScalarDataTypes() const = 0;
	virtual unsigned int GetNumVectorDataTypes() const = 0;

protected:
	ICallOutHandler* coh;
};

// engine gets and frees a module instance through these; implemented library-side
extern "C" SLIB_EXPORT   IPathModule*   CALL_CONV   GetPathModuleInstance(ICallOutHandler*);
extern "C" SLIB_EXPORT   void           CALL_CONV   FreePathModuleInstance(IPathModule*);

#endif
