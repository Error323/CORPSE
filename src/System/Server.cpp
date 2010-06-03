#include <SDL/SDL_timer.h>

#include "./Server.hpp"
#include "./EngineAux.hpp"
#include "./LuaParser.hpp"
#include "./NetMessageBuffer.hpp"

#define MMAX(a, b) (((a) > (b))? (a): (b));

CServer* CServer::GetInstance() {
	static CServer* s = NULL;
	static unsigned int depth = 0;

	if (s == NULL) {
		assert(depth == 0);

		depth += 1;
		s = new CServer();
		depth -= 1;
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
	pauseTickDelta = 0;
	missedFrames   = 0;

	const LuaTable* rootTable = LUA->GetRoot();
	const LuaTable* generalTable = rootTable->GetTblVal("general");

	simFrameRate = unsigned(generalTable->GetFltVal("simFrameRate", 25));
	simFrameTime = 1000 / simFrameRate;
	simFrameMult = unsigned(generalTable->GetFltVal("simRateMult", 1));
}

void CServer::ChangeSpeed(uint mult) {
	if ((mult > 0) && ((1000 / (simFrameRate * mult)) > 0)) {
		simFrameMult = mult;
		simFrameTime = 1000 / (simFrameRate * simFrameMult);
	}
}



void CServer::ReadNetMessages() {
	NetMessage m;

	while (netBuf->PopClientToServerMessage(&m)) {
		switch (m.GetID()) {
			case CLIENT_MSG_PAUSE: {
				paused = !paused;

				if (paused) {
					pauseTickDelta = SDL_GetTicks() - lastTick;
				}
			} break;

			case CLIENT_MSG_INCSIMSPEED: {
				ChangeSpeed(simFrameMult + 1);
			} break;
			case CLIENT_MSG_DECSIMSPEED: {
				ChangeSpeed(simFrameMult - 1);
			} break;

			case CLIENT_MSG_COMMAND: {
				SendNetMessage(m); // broadcast
			}
		}
	}
}

void CServer::SendNetMessage(const NetMessage& m) {
	/*
	for each client {
		netBuf[client]->AddServerToClientMessage(m);
	}
	*/

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
			SendNetMessage(NetMessage(SERVER_MSG_SIMFRAME, 0));

			gameTime      = frame / simFrameRate;           // game-time is based on number of elapsed frames
			frame        += 1;                              // update the server's internal frame number
			missedFrames  = MMAX(0, missedFrames - 1);      // we made up one frame of our backlog
			missedFrames += (frameTime / simFrameTime);     // update how many frames we are still "behind"
			updated       = true;
		} else {
			frameTime = 0;
		}

		lastTick        = SDL_GetTicks();
		realTime        = (lastTick - startTime) / 1000;    // update the program's real running time (inc. pauses)
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



unsigned int CServer::GetLastTickDelta() const {
	return ((!paused)? (SDL_GetTicks() - lastTick): pauseTickDelta);
}
