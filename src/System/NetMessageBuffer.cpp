#include <cassert>
#include "./NetMessageBuffer.hpp"

CNetMessageBuffer* CNetMessageBuffer::GetInstance() {
	static CNetMessageBuffer* nmb = NULL;
	static unsigned int depth = 0;

	if (nmb == NULL) {
		assert(depth == 0);

		depth += 1;
		nmb = new CNetMessageBuffer();
		depth -= 1;
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

bool CNetMessageBuffer::PopClientToServerMessage(NetMessage* m) {
	if (clientServerMsgs.empty()) {
		return false;
	} else {
		*m = clientServerMsgs.front();
		clientServerMsgs.pop_front();
		return true;
	}
}
bool CNetMessageBuffer::PopServerToClientMessage(NetMessage* m) {
	if (serverClientMsgs.empty()) {
		return false;
	} else {
		*m = serverClientMsgs.front();
		serverClientMsgs.pop_front();
		return true;
	}
}


bool CNetMessageBuffer::PeekClientToServerMessage(NetMessage* m) const {
	if (clientServerMsgs.empty()) {
		return false;
	} else {
		*m = clientServerMsgs.front();
		return true;
	}
}
bool CNetMessageBuffer::PeekServerToClientMessage(NetMessage* m) const {
	if (serverClientMsgs.empty()) {
		return false;
	} else {
		*m = serverClientMsgs.front();
		return true;
	}
}


void CNetMessageBuffer::AddClientToServerMessage(const NetMessage& m) { clientServerMsgs.push_back(m); }
void CNetMessageBuffer::AddServerToClientMessage(const NetMessage& m) { serverClientMsgs.push_back(m); }
