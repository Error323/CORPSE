#ifndef PFFG_CLIENT_HDR
#define PFFG_CLIENT_HDR

#include "../Input/InputReceiver.hpp"

struct SDL_Surface;
union SDL_Event;

class CInputHandler;
class CSimThread;
class CRenderThread;

class CClient: public CInputReceiver {
public:
	static CClient* GetInstance(int, char**);
	static void FreeInstance(CClient*);

	void ParseArgs(int, char**);
	void Update();

	void MouseMoved(int, int, int, int);
	void MousePressed(int, int, int, bool);
	void MouseReleased(int, int, int);
	void KeyPressed(int, bool);
	void WindowResized(int, int, int);
	void WindowExposed();

private:
	CClient(int, char**);
	~CClient();

	void ReadNetMessages();
	void SendNetMessage(int);

	void Init();
	void InitSDL(const char*);

	bool EnableMultiSampling();
	bool VerifyMultiSampling();

	void InitStencilBuffer();
	void SetWindowSize(int, int);

	void SetSDLWindowVideoMode();
	void SetOGLViewPortGeometry();

	// helper functions for SetOGLViewPortGeometry()
	void UpdateViewPortDimensions();
	bool UpdateWindowInfo();

	SDL_Surface* mScreen;

	CInputHandler* mInputHandler;
	CSimThread* mSimThread;
	CRenderThread* mRenderThread;
};

#endif
