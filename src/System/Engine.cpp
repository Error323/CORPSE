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
	mEngineAux = EngineAux::GetInstance(argc, argv);

	mEventHandler = EventHandler::GetInstance();

	mServer = CServer::GetInstance();
	mClient = CClient::GetInstance(argc, argv);

	mClient->SetClientID(mServer->GetNumClients());
	mServer->AddNetMessageBuffer(mClient->GetClientID());

	mClient->SetNetMessageBuffer(mServer->GetNetMessageBuffer(mClient->GetClientID()));
}

CEngine::~CEngine() {
	mServer->DelNetMessageBuffer(mClient->GetClientID());

	CServer::FreeInstance(mServer);
	CClient::FreeInstance(mClient);

	EventHandler::FreeInstance(mEventHandler);
	EngineAux::FreeInstance(mEngineAux);
}

void CEngine::Run() {
	while (!AUX->GetWantQuit()) {
		mServer->Update();
		mClient->Update();
	}
}
