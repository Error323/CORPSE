#include <cstdlib>
#include <ctime>

#ifndef PFFG_SERVER_NOTHREAD
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#endif

#include "./Engine.hpp"
#include "./EngineAux.hpp"
#include "./LuaParser.hpp"
#include "./Client.hpp"
#include "./Server.hpp"
#include "./EventHandler.hpp"
#include "./NetMessageBuffer.hpp"
#include "./Debugger.hpp"

CEngine* CEngine::GetInstance(int argc, char** argv) {
	static CEngine* e = NULL;
	static unsigned int depth = 0;

	if (e == NULL) {
		PFFG_ASSERT(depth == 0);

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

	// needs to be initialized after mClient
	mDebugger = Debugger::GetInstance();

	mClient->SetClientID(mServer->GetNumClients());
	mServer->AddNetMessageBuffer(mClient->GetClientID());

	mClient->SetNetMessageBuffer(mServer->GetNetMessageBuffer(mClient->GetClientID()));
}

CEngine::~CEngine() {
	mServer->DelNetMessageBuffer(mClient->GetClientID());

	Debugger::FreeInstance(mDebugger);

	CServer::FreeInstance(mServer);
	CClient::FreeInstance(mClient);

	EventHandler::FreeInstance(mEventHandler);
	EngineAux::FreeInstance(mEngineAux);
}

void CEngine::Run() {
	#ifndef PFFG_SERVER_NOTHREAD
	boost::thread serverThread(boost::bind(&CServer::Run, mServer));

	while (!AUX->GetWantQuit()) {
		mClient->Update();
	}

	serverThread.join();
	#else
	while (!AUX->GetWantQuit()) {
		mServer->Update();
		mClient->Update();
	}
	#endif
}
