#include <SDL/SDL_timer.h>

#include "./Server.hpp"
#include "./EngineAux.hpp"
#include "./LuaParser.hpp"
#include "./NetMessageBuffer.hpp"

#define MMAX(a, b) (((a) > (b))? (a): (b));

CServer* CServer::GetInstance() {
	CServer* s = NULL;

	if (s == NULL) {
		s = new CServer();
	}

	return s;
}

void CServer::FreeInstance(CServer* s) {
	delete s;
}



CServer::CServer() {
	paused         = false;

	frame          = 0;
	frameTime      = 0;
	frameDelta     = 0;
	realTime       = 0;
	prevRealTime   = 0;
	startTime      = SDL_GetTicks();
	gameTime       = 0;
	lastTick       = 0;
	missedFrames   = 0;

	const LuaTable* rootTable = LUA->GetRoot();
	const LuaTable* generalTable = rootTable->GetTblVal("general");

	simFrameRate = unsigned(generalTable->GetFltVal("simFrameRate", 25));
	simFrameTime = 1000 / simFrameRate;
	simFrameMult = unsigned(generalTable->GetFltVal("simRateMult", 1));
}

void CServer::ChangeSpeed(uint mult) {
	simFrameMult = mult;
	simFrameTime = 1000 / (simFrameRate * simFrameMult);
}



void CServer::ReadNetMessages() {
	int m = CLIENT_MSG_NONE;

	while (netBuf->PopClientToServerMessage(&m)) {
		if (m == CLIENT_MSG_PAUSE) {
			paused = !paused;
		}
	}
}

void CServer::SendNetMessage(int m) {
	// FIXME: needs to be done for each
	// client link (local loopback conn?)
	netBuf->AddServerToClientMessage(m);
}



bool CServer::Update() {
	ReadNetMessages();

	bool updated = false;

	if (frameDelta <= 0 || missedFrames > 0 /*|| gameTime < realTime*/) {
		// previous frame took longer than simFrameTime or it is
		// time to execute a new one; if we missed any "scheduled"
		// frames due to some unusually long frameTime, catch up by
		// creating a new one too
		if (!paused) {
			SendNetMessage(SERVER_MSG_SIMFRAME);

			gameTime      = frame / simFrameRate;           // game-time is based on number of elapsed frames
			frame        += 1;                              // update the server's internal frame number
			missedFrames  = MMAX(0, missedFrames - 1);      // we made up one frame of our backlog
			missedFrames += (frameTime / simFrameTime);     // update how many frames we are still "behind"
			updated       = true;
		} else {
			frameTime = 0;
		}

		lastTick        = SDL_GetTicks();
		realTime        = (lastTick - startTime) / 1000;    // update the program's real running time
		frameDelta      = simFrameTime - frameTime;         // how much time we had left to complete this frame
		totalFrameTime += frameTime;                        // update the cumulative frame-time

		if ((realTime - prevRealTime) >= 1) {
			// (at least) one real-time second's worth of sim-frames passed
			// which together took <totalFrameTime> milliseconds to calculate;
			// if framerate was not changed then simFPS will be APPROXIMATELY
			// equal to <FRAMERATE>, but in general we do not know how many
			// have been calculated between <prevRealTime> and <realTime>
			// (so totalFrameTime cannot be divided by fixed <FRAMERATE>)
			//
			// note: checking if <totalFrameTime> >= 1000 is pointless since
			// bigger framerate means totalFrameTime will increase faster (so
			// correspondence with true running time <realTime> lost, updates
			// will then occur every 1000 / <per-frame time> frames)
			prevRealTime = realTime;
		}
	} else {
		// last frame took less than FRAMETIME, if it is time for a
		// new one then reset frameDelta to 0 so next Update() call
		// will create one
		if (SDL_GetTicks() >= (lastTick + frameDelta)) {
			frameDelta = 0;
		}
	}

	return updated;
}
