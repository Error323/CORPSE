#ifndef PFFG_ENGINE_HDR
#define PFFG_ENGINE_HDR

class CNetMessageBuffer;
class EventHandler;
class CClient;
class CServer;

class CEngine {
public:
	static CEngine* GetInstance(int, char**);
	static void FreeInstance(CEngine*);

	void Run();

private:
	CEngine(int, char**);
	~CEngine();

	EventHandler* mEventHandler;

	CClient* mClient;
	CServer* mServer;
};

#endif
