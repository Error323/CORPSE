#ifndef PFFG_NETMESSAGES_HDR
#define PFFG_NETMESSAGES_HDR

#include <cassert>
#include <cstring>
#include <vector> 

// messages originating from client
enum ClientMessageIDs {
	CLIENT_MSG_PAUSE       =  0,
	CLIENT_MSG_INCSIMSPEED =  1,
	CLIENT_MSG_DECSIMSPEED =  2,
	CLIENT_MSG_COMMAND     =  3,
};

// messages originating from server
enum ServerMessageIDs {
	SERVER_MSG_SIMFRAME = 0
};



struct NetMessage {
public:
	NetMessage(): id(-1), pos(-1) {}
	NetMessage(unsigned int msgID, unsigned int size): id(msgID), pos(0) {
		data.resize(size);
	}
	~NetMessage() {
		data.clear();
	}

	NetMessage& operator = (const NetMessage& m) {
		id   = m.id;
		pos  = m.pos;
		data = m.data;

		return *this;
	}

	template<typename T> NetMessage& operator << (T t) {
		assert((pos + sizeof(T)) <= data.size());
		memcpy(&data[pos], reinterpret_cast<unsigned char*>(&t), sizeof(T));
		pos += sizeof(T);
		return *this;
	}
	// note: extracts in reverse-order
	template<typename T> NetMessage& operator >> (T& t) {
		pos -= sizeof(T);
		assert((pos + sizeof(T)) <= data.size());
		t = *(reinterpret_cast<T*>(&data[pos]));
		return *this;
	}

	unsigned int GetID() const { return id; }

private:
	unsigned int id;
	unsigned int pos;

	std::vector<unsigned char> data;
};

#endif
