#ifndef PFFG_SERVER_HDR
#define PFFG_SERVER_HDR

class CServer {
public:
	static CServer* GetInstance();
	static void FreeInstance(CServer*);

	bool Update();

	void ChangeSpeed(unsigned int);
	bool IsPaused() const { return paused; }
	void TogglePause() { paused = !paused; }

	unsigned int GetSimFrameRate() const { return simFrameRate; }
	unsigned int GetSimFrameTime() const { return simFrameTime; }
	unsigned int GetSimFrameMult() const { return simFrameMult; }

private:
	CServer();
	~CServer() {}

	void ReadNetMessages();
	void SendNetMessage(int);

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
	unsigned int lastTick;
	int          missedFrames;              // num. of frames delayed (due to high frameTime) by the last frame

	unsigned int simFrameRate;              // current simulation speed (number of simframes processed per real-time second)
	unsigned int simFrameTime;              // ideal maximum amount of time a single sim-frame may take at current speed
	unsigned int simFrameMult;              // simulation speed multiplier
};

#define server (CServer::GetInstance())

#endif
