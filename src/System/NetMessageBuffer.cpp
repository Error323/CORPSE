#ifndef PFFG_SERVER_NOTHREAD
#include <boost/thread/mutex.hpp>
#endif

#include "./NetMessageBuffer.hpp"

CNetMessageBuffer::CNetMessageBuffer() {
        #ifndef PFFG_SERVER_NOTHREAD
        msgMutex = new boost::mutex::mutex();
        #endif
}

CNetMessageBuffer::~CNetMessageBuffer() {
	clientServerMsgs.clear();
	serverClientMsgs.clear();

	#ifndef PFFG_SERVER_NOTHREAD
	delete msgMutex;
	#endif
}



bool CNetMessageBuffer::PopClientToServerMessage(NetMessage* m) {
	#ifndef PFFG_SERVER_NOTHREAD
	boost::mutex::scoped_lock lock(*msgMutex);
	#endif

	if (clientServerMsgs.empty()) {
		return false;
	} else {
		*m = clientServerMsgs.front();
		clientServerMsgs.pop_front();
		return true;
	}
}
bool CNetMessageBuffer::PopServerToClientMessage(NetMessage* m) {
	#ifndef PFFG_SERVER_NOTHREAD
	boost::mutex::scoped_lock lock(*msgMutex);
	#endif

	if (serverClientMsgs.empty()) {
		return false;
	} else {
		*m = serverClientMsgs.front();
		serverClientMsgs.pop_front();
		return true;
	}
}


bool CNetMessageBuffer::PeekClientToServerMessage(NetMessage* m) const {
	#ifndef PFFG_SERVER_NOTHREAD
	boost::mutex::scoped_lock lock(*msgMutex);
	#endif

	if (clientServerMsgs.empty()) {
		return false;
	} else {
		*m = clientServerMsgs.front();
		return true;
	}
}
bool CNetMessageBuffer::PeekServerToClientMessage(NetMessage* m) const {
	#ifndef PFFG_SERVER_NOTHREAD
	boost::mutex::scoped_lock lock(*msgMutex);
	#endif

	if (serverClientMsgs.empty()) {
		return false;
	} else {
		*m = serverClientMsgs.front();
		return true;
	}
}


void CNetMessageBuffer::AddClientToServerMessage(const NetMessage& m) {
	#ifndef PFFG_SERVER_NOTHREAD
	boost::mutex::scoped_lock lock(*msgMutex);
	#endif
	clientServerMsgs.push_back(m);
}
void CNetMessageBuffer::AddServerToClientMessage(const NetMessage& m) {
	#ifndef PFFG_SERVER_NOTHREAD
	boost::mutex::scoped_lock lock(*msgMutex);
	#endif
	serverClientMsgs.push_back(m);
}



unsigned int CNetMessageBuffer::GetServerToClientMessageCount(unsigned int msgID) {
	#ifndef PFFG_SERVER_NOTHREAD
	boost::mutex::scoped_lock lock(*msgMutex);
	#endif

	unsigned int n = 0;

	for (MsgQueue::iterator it = serverClientMsgs.begin(); it != serverClientMsgs.end(); ++it) {
		if ((*it).GetMessageID() == msgID) {
			n += 1;
		}
	}

	return n;
}

unsigned int CNetMessageBuffer::GetClientToServerMessageCount(unsigned int msgID) {
	#ifndef PFFG_SERVER_NOTHREAD
	boost::mutex::scoped_lock lock(*msgMutex);
	#endif

	unsigned int n = 0;

	for (MsgQueue::iterator it = clientServerMsgs.begin(); it != clientServerMsgs.end(); ++it) {
		if ((*it).GetMessageID() == msgID) {
			n += 1;
		}
	}

	return n;
}
