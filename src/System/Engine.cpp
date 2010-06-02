#include <cstdlib>
#include <ctime>

#include "./Engine.hpp"
#include "./EngineAux.hpp"
#include "./LuaParser.hpp"
#include "./Client.hpp"
#include "./Server.hpp"
#include "./EventHandler.hpp"
#include "./NetMessageBuffer.hpp"

CEngine* CEngine::GetInstance(int argc, char** argv) {
	static CEngine* e = NULL;
	static unsigned int depth = 0;

	if (e == NULL) {
		assert(depth == 0);

		depth += 1;
		e = new CEngine(argc, argv);
		depth -= 1;
	}

	return e;
}

void CEngine::FreeInstance(CEngine* e) {
	delete e;
}



CEngine::CEngine(int argc, char** argv) {
	EngineAux::Init(argc, argv);

	mNetBuf = CNetMessageBuffer::GetInstance();
	mEventHandler = EventHandler::GetInstance();

	mServer = CServer::GetInstance();
	mClient = CClient::GetInstance(argc, argv);
}

CEngine::~CEngine() {
	CServer::FreeInstance(mServer);
	CClient::FreeInstance(mClient);

	EventHandler::FreeInstance(mEventHandler);
	CNetMessageBuffer::FreeInstance(mNetBuf);
}

void CEngine::Run() {
	while (!ENG->GetWantQuit()) {
		mServer->Update();
		mClient->Update();
	}
}
