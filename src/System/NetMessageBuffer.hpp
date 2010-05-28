#ifndef PFFG_NETMESSAGEBUFFER_HDR
#define PFFG_NETMESSAGEBUFFER_HDR

#include <list>
#include "./NetMessages.hpp"

typedef std::list<int> MsgQueue;

class CNetMessageBuffer {
public:
	static CNetMessageBuffer* GetInstance();
	static void FreeInstance(CNetMessageBuffer*);

	bool PopClientToServerMessage(int*);
	bool PopServerToClientMessage(int*);

	bool PeekClientToServerMessage(int*) const;
	bool PeekServerToClientMessage(int*) const;

	void AddClientToServerMessage(int);
	void AddServerToClientMessage(int);

private:
	CNetMessageBuffer() {}
	~CNetMessageBuffer();

	MsgQueue clientServerMsgs;   // client-to-server
	MsgQueue serverClientMsgs;   // server-to-client(s)
};

#define netBuf (CNetMessageBuffer::GetInstance())

#endif
