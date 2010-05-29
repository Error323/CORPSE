#ifndef PFFG_SIMTHREAD_HDR
#define PFFG_SIMTHREAD_HDR

class CGround;
class CReadMap;
class CMapInfo;

class SimObjectHandler;
class CSimThread {
public:
	static CSimThread* GetInstance();
	static void FreeInstance(CSimThread*);

	void Update();

private:
	CSimThread();
	~CSimThread();

	CGround* mGround;
	CReadMap* mReadMap;
	const CMapInfo* mMapInfo;

	SimObjectHandler* mSimObjectHandler;
};

#endif
