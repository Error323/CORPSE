#ifndef PFFG_RENDERTHREAD_HDR
#define PFFG_RENDERTHREAD_HDR

#include <list>
#include "../Math/vec3fwd.hpp"

class CCameraController;
class CScene;

class CRenderThread {
public:
	static CRenderThread* GetInstance();
	static void FreeInstance(CRenderThread*);

	void Update();

	unsigned int GetFrame() const { return frame; }
	CCameraController* GetCamCon() const { return camCon; }

private:
	CRenderThread();
	~CRenderThread();

	CCameraController* camCon;
	CScene* scene;

	// current renderer-frame
	unsigned int frame;
};

#define rThread (CRenderThread::GetInstance())

#endif
