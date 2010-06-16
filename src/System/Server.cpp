#include <iostream>
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
	const LuaTable* serverTable = rootTable->GetTblVal("server");

	simFrameRate = unsigned(serverTable->GetFltVal("simFrameRate", 5));
	simFrameMult = unsigned(serverTable->GetFltVal("simRateMult", 5));
	simFrameTime = 1000 / (simFrameRate * simFrameMult);
}



void CServer::AddNetMessageBuffer(unsigned int clientID) {
	netBufs[clientID] = new CNetMessageBuffer();
	clientFrames[clientID] = 0;
}

void CServer::DelNetMessageBuffer(unsigned int clientID) {
	delete netBufs[clientID];
	netBufs.erase(clientID);
	clientFrames.erase(clientID);
}



void CServer::ChangeSpeed(uint mult) {
	if ((mult > 0) && ((1000 / (simFrameRate * mult)) > 0)) {
		simFrameMult = mult;
		simFrameTime = 1000 / (simFrameRate * simFrameMult);

		std::cout << "[CServer::ChangeSpeed][frame=" << frame << "]";
		std::cout << " speed-multiplier set to " << simFrameMult;
		std::cout << " (frame-rate: " << (simFrameRate * simFrameMult) << "fps";
		std::cout << ", frame-time: " << (simFrameTime               ) << "ms)";
		std::cout << std::endl;

		/*
		// clients cannot fall behind the server yet
		// (since they are part of the same process)
		for (unsigned int i = 0; i < GetNumClients(); i++) {
			std::cout << "\tclient " << i << " is at sim-frame " << clientFrames[i];
			std::cout << " (lag: " << (frame - clientFrames[i]) << " frames)" << std::endl;
		}
		*/
	}
}



void CServer::ReadNetMessages() {
	for (std::map<unsigned int, CNetMessageBuffer*>::iterator it = netBufs.begin(); it != netBufs.end(); ++it) {
		CNetMessageBuffer* clientMsgBuf = it->second;
		NetMessage m;

		while (clientMsgBuf->PopClientToServerMessage(&m)) {
			switch (m.GetMessageID()) {
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

				case CLIENT_MSG_SIMFRAME: {
					clientFrames[m.GetSenderID()] += 1;
				} break;

				case CLIENT_MSG_SIMCOMMAND: {
					SendNetMessage(m); // broadcast
				} break;
			}
		}
	}
}

void CServer::SendNetMessage(const NetMessage& m) {
	for (std::map<unsigned int, CNetMessageBuffer*>::iterator it = netBufs.begin(); it != netBufs.end(); ++it) {
		CNetMessageBuffer* clientMsgBuf = it->second;
		clientMsgBuf->AddServerToClientMessage(m);
	}
}



bool CServer::Update() {
	ReadNetMessages();

	bool updated = false;

	if (frameDelta <= 0 || missedFrames > 0 /*|| gameTime < realTime*/) {
		if (!paused) {
			SendNetMessage(NetMessage(SERVER_MSG_SIMFRAME, 0xDEADF00D, 0));

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
		if (SDL_GetTicks() >= (lastTick + frameDelta)) {
			// time for a new frame
			frameDelta = 0;
		}
	}

	return updated;
}

#ifndef PFFG_SERVER_NOTHREAD
void CServer::Run() {
	while (!AUX->GetWantQuit()) {
		Update();
	}
}
#endif



unsigned int CServer::GetLastTickDelta() const {
	return ((!paused)? (SDL_GetTicks() - lastTick): pauseTickDelta);
}
