#ifndef PFFG_NETMESSAGEBUFFER_HDR
#define PFFG_NETMESSAGEBUFFER_HDR

#include <list>
#include "./NetMessages.hpp"

#define PFFG_SERVER_NOTHREAD 1
#ifndef PFFG_SERVER_NOTHREAD
namespace boost {
	 class mutex;
};
#endif

typedef std::list<NetMessage> MsgQueue;

class CNetMessageBuffer {
public:
	CNetMessageBuffer();
	~CNetMessageBuffer();

	bool PopClientToServerMessage(NetMessage*);
	bool PopServerToClientMessage(NetMessage*);

	bool PeekClientToServerMessage(NetMessage*) const;
	bool PeekServerToClientMessage(NetMessage*) const;

	// these deep-copy the message
	void AddClientToServerMessage(const NetMessage&);
	void AddServerToClientMessage(const NetMessage&);

	unsigned int GetServerToClientMessageCount(unsigned int);
	unsigned int GetClientToServerMessageCount(unsigned int);

private:
	MsgQueue clientServerMsgs;   // client-to-server
	MsgQueue serverClientMsgs;   // server-to-client(s)

	#ifndef PFFG_SERVER_NOTHREAD
	boost::mutex* msgMutex;
	#endif
};

#define netBuf (CNetMessageBuffer::GetInstance())

#endif
