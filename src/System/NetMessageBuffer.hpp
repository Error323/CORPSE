#ifndef PFFG_NETMESSAGEBUFFER_HDR
#define PFFG_NETMESSAGEBUFFER_HDR

#include <list>
#include "./NetMessages.hpp"

typedef std::list<NetMessage> MsgQueue;

class CNetMessageBuffer {
public:
	static CNetMessageBuffer* GetInstance();
	static void FreeInstance(CNetMessageBuffer*);

	bool PopClientToServerMessage(NetMessage*);
	bool PopServerToClientMessage(NetMessage*);

	bool PeekClientToServerMessage(NetMessage*) const;
	bool PeekServerToClientMessage(NetMessage*) const;

	// these deep-copy the message
	void AddClientToServerMessage(const NetMessage&);
	void AddServerToClientMessage(const NetMessage&);

private:
	CNetMessageBuffer() {}
	~CNetMessageBuffer();

	MsgQueue clientServerMsgs;   // client-to-server
	MsgQueue serverClientMsgs;   // server-to-client(s)
};

#define netBuf (CNetMessageBuffer::GetInstance())

#endif
