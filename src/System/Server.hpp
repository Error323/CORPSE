#ifndef PFFG_SERVER_HDR
#define PFFG_SERVER_HDR

#include <map>

class CNetMessageBuffer;
struct NetMessage;

class CServer {
public:
	static CServer* GetInstance();
	static void FreeInstance(CServer*);

	void AddNetMessageBuffer(unsigned int);
	void DelNetMessageBuffer(unsigned int);

	// note: these two are not thread-safe at run-time
	unsigned int GetNumClients() const { return netBufs.size(); }
	CNetMessageBuffer* GetNetMessageBuffer(unsigned int clientID) { return netBufs[clientID]; }

	void SendNetMessage(const NetMessage&);

	bool Update();
	#ifndef PFFG_SERVER_NOTHREAD
	void Run();
	#endif

	// note: only used for client-side interpolation
	// between frames, so it does not require locking
	float GetLastTickDeltaRatio() const { return (GetLastTickDelta() / (1000.0f / simFrameRate)); }

	unsigned int GetSimFrameRate() const { return simFrameRate; }
	unsigned int GetSimFrameMult() const { return simFrameMult; }
	unsigned int GetSimFrameTime() const { return simFrameTime; }

private:
	CServer();
	~CServer() {}

	void ChangeSpeed(unsigned int);
	void ReadNetMessages();
	unsigned int GetLastTickDelta() const;

	bool paused;

	unsigned int frame;                     // current server frame
	int          frameDelta;                // time left over after calculating <f> (millisecs, should be > 0)
	unsigned int avgFrameTime;              // average time per frame, set every real-time second
	unsigned int realTime;                  // number of real-time seconds elapsed so far
	unsigned int prevRealTime;              // previous real-time snapshot
	unsigned int startTime;                 // program start-time
	unsigned int gameTime;                  // number of game-time seconds elapsed so far
	unsigned int lastTick;                  // time-stamp in ticks of last-executed sim-frame
	unsigned int pauseTickDelta;            // snapshot of GetLastTickDelta() when simulation was last paused
	int          missedFrames;              // num. of frames delayed (due to high frameTime) by the last frame

	unsigned int simFrameRate;              // current simulation speed (number of sim-frames per real-time second at speed=1)
	unsigned int simFrameMult;              // simulation speed multiplier
	unsigned int simFrameTime;              // ideal maximum amount of time a single sim-frame may take at current speed (ms)

	std::map<unsigned int, unsigned int> clientFrames;
	std::map<unsigned int, CNetMessageBuffer*> netBufs;
};

#define server (CServer::GetInstance())

#endif
