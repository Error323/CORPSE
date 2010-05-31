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

	if (e == NULL) {
		e = new CEngine(argc, argv);
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
