#ifndef PFFG_SIMTHREAD_HDR
#define PFFG_SIMTHREAD_HDR

class CGround;
class CReadMap;
class CMapInfo;

class IPathModule;
struct NetMessage;

class SimObjectHandler;
class CSimThread {
public:
	static CSimThread* GetInstance();
	static void FreeInstance(CSimThread*);

	void Update();
	void SimCommand(NetMessage&);

	unsigned int GetFrame() const { return frame; }
	const IPathModule* GetPathModule() const { return mPathModule; }

private:
	CSimThread();
	~CSimThread();

	CGround* mGround;
	CReadMap* mReadMap;
	const CMapInfo* mMapInfo;

	SimObjectHandler* mSimObjectHandler;
	IPathModule* mPathModule;

	// current simulation-frame
	unsigned int frame;
};

#define simThread (CSimThread::GetInstance())

#endif
