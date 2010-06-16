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

	// note:
	//   these are only used for client-side interpolation
	//   between sim-frames, so they do not require locking
	unsigned int GetLastTickDelta() const;
	float GetLastTickDeltaRatio() const { return (GetLastTickDelta() / float(simFrameTime)); }

	unsigned int GetSimFrameRate() const { return simFrameRate; }
	unsigned int GetSimFrameTime() const { return simFrameTime; }
	unsigned int GetSimFrameMult() const { return simFrameMult; }

private:
	CServer();
	~CServer() {}

	void ChangeSpeed(unsigned int);
	void ReadNetMessages();

	bool paused;

	unsigned int frame;                     // current server frame
	unsigned int frameTime;                 // time taken to calculate sim-frame <f> (millisecs)
	int          frameDelta;                // time left over after calculating <f> (millisecs, should be > 0)
	unsigned int totalFrameTime;            // total time taken by all frames between realTime and prevRealTime
	unsigned int avgFrameTime;              // average time per frame, set every real-time second
	unsigned int realTime;                  // number of real-time seconds elapsed so far
	unsigned int prevRealTime;              // previous real-time snapshot
	unsigned int startTime;                 // program start-time
	unsigned int gameTime;                  // number of game-time seconds elapsed so far
	unsigned int lastTick;                  // time-stamp in ticks of last-executed sim-frame
	unsigned int pauseTickDelta;            // snapshot of GetLastTickDelta() when simulation was last paused
	int          missedFrames;              // num. of frames delayed (due to high frameTime) by the last frame

	unsigned int simFrameRate;              // current simulation speed (number of simframes processed per real-time second)
	unsigned int simFrameTime;              // ideal maximum amount of time a single sim-frame may take at current speed (ms)
	unsigned int simFrameMult;              // simulation speed multiplier

	std::map<unsigned int, unsigned int> clientFrames;
	std::map<unsigned int, CNetMessageBuffer*> netBufs;
};

#define server (CServer::GetInstance())

#endif
