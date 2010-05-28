#include "./NetMessageBuffer.hpp"

CNetMessageBuffer* CNetMessageBuffer::GetInstance() {
	static CNetMessageBuffer* nmb = NULL;

	if (nmb == NULL) {
		nmb = new CNetMessageBuffer();
	}

	return nmb;
}

void CNetMessageBuffer::FreeInstance(CNetMessageBuffer* nmb) {
	delete nmb;
}



CNetMessageBuffer::~CNetMessageBuffer() {
	clientServerMsgs.clear();
	serverClientMsgs.clear();
}

bool CNetMessageBuffer::PopClientToServerMessage(int* m) {
	if (clientServerMsgs.empty()) {
		*m = CLIENT_MSG_NONE;
		return false;
	} else {
		*m = clientServerMsgs.front();
		clientServerMsgs.pop_front();
		return true;
	}
}
bool CNetMessageBuffer::PopServerToClientMessage(int* m) {
	if (serverClientMsgs.empty()) {
		*m = SERVER_MSG_NONE;
		return false;
	} else {
		*m = serverClientMsgs.front();
		serverClientMsgs.pop_front();
		return true;
	}
}


bool CNetMessageBuffer::PeekClientToServerMessage(int* m) const {
	if (clientServerMsgs.empty()) {
		*m = CLIENT_MSG_NONE;
		return false;
	} else {
		*m = clientServerMsgs.front();
		return true;
	}
}
bool CNetMessageBuffer::PeekServerToClientMessage(int* m) const {
	if (serverClientMsgs.empty()) {
		*m = SERVER_MSG_NONE;
		return false;
	} else {
		*m = serverClientMsgs.front();
		return true;
	}
}


void CNetMessageBuffer::AddClientToServerMessage(int m) {
	clientServerMsgs.push_back(m);
}
void CNetMessageBuffer::AddServerToClientMessage(int m) {
	serverClientMsgs.push_back(m);
}
