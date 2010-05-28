#ifndef PFFG_SERVER_HDR
#define PFFG_SERVER_HDR

class CServer {
public:
	static CServer* GetInstance();
	static void FreeInstance(CServer*);

	bool Update();

	void ChangeSpeed(uint);
	bool IsPaused() const { return paused; }
	void TogglePause() { paused = !paused; }

private:
	CServer();
	~CServer() {}

	void ReadNetMessages();
	void SendNetMessage(int);

	bool paused;

	uint frame;                     // current server frame
	uint frameTime;                 // time taken to calculate sim-frame <f> (millisecs)
	int  frameDelta;                // time left over after calculating <f> (millisecs, should be > 0)
	uint totalFrameTime;            // total time taken by all frames between realTime and prevRealTime
	uint avgFrameTime;              // average time per frame, set every real-time second
	uint realTime;                  // number of real-time seconds elapsed so far
	uint prevRealTime;              // previous real-time snapshot
	uint startTime;                 // program start-time
	uint gameTime;                  // number of game-time seconds elapsed so far
	uint lastTick;
	int  missedFrames;              // num. of frames delayed (due to high frameTime) by the last frame

	uint simFrameRate;              // current simulation speed (number of simframes processed per real-time second)
	uint simFrameTime;              // ideal maximum amount of time a single sim-frame may take at current speed
	uint simFrameMult;              // simulation speed multiplier
};

#endif

