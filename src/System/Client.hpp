#ifndef PFFG_CLIENT_HDR
#define PFFG_CLIENT_HDR

#include "../Input/InputReceiver.hpp"

struct SDL_Surface;
union SDL_Event;

class CInputHandler;
class CSimThread;
class CRenderThread;

class IWindow;
class UI;

class CNetMessageBuffer;
struct NetMessage;

class CClient: public CInputReceiver {
public:
	static CClient* GetInstance(int, char**);
	static void FreeInstance(CClient*);

	void SetClientID(unsigned int id) { clientID = id; }
	unsigned int GetClientID() const { return clientID; }

	void SetNetMessageBuffer(CNetMessageBuffer* buf) { mNetBuf = buf; }
	void SendNetMessage(const NetMessage&);

	void Update();

	void KeyPressed(int, bool);
	void WindowResized(int, int);
	void WindowExposed();

private:
	CClient(int, char**);
	~CClient();

	void ReadNetMessages();

	void InitSDL();
	void KillSDL();

	IWindow* mWindow;
	UI* mUI;

	CInputHandler* mInputHandler;
	CSimThread* mSimThread;
	CRenderThread* mRenderThread;

	unsigned int clientID;

	// bi-directional comm. channel to server
	CNetMessageBuffer* mNetBuf;
};

#define client (CClient::GetInstance(0, NULL))

#endif
