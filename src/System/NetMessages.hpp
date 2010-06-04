#ifndef PFFG_NETMESSAGES_HDR
#define PFFG_NETMESSAGES_HDR

#include <cassert>
#include <cstring>
#include <vector> 

// messages originating from client
enum ClientMessageIDs {
	CLIENT_MSG_PAUSE       =  1,
	CLIENT_MSG_INCSIMSPEED =  2,
	CLIENT_MSG_DECSIMSPEED =  3,
	CLIENT_MSG_COMMAND     =  4,
};

// messages originating from server
enum ServerMessageIDs {
	SERVER_MSG_SIMFRAME = 1
};



struct NetMessage {
public:
	NetMessage(): id(0), pos(0) {}
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
	template<typename T> NetMessage& operator << (const std::vector<T>& v) {
		for (typename std::vector<T>::const_iterator it = v.begin(); it != v.end(); ++it) {
			(*this) << (*it);
		}

		return *this;
	}

	// note: extracts in reverse-order
	template<typename T> NetMessage& operator >> (T& t) {
		pos -= sizeof(T);
		assert((pos + sizeof(T)) <= data.size());
		t = *(reinterpret_cast<T*>(&data[pos]));
		return *this;
	}
	// note: <v> must have as many elements as the
	// corresponding vector passed to operator <<
	template<typename T> NetMessage& operator >> (std::vector<T>& v) {
		for (typename std::vector<T>::iterator it = v.begin(); it != v.end(); ++it) {
			(*this) >> (*it);
		}
		return *this;
	}

	unsigned int GetID() const { return id; }

private:
	unsigned int id;
	unsigned int pos;

	std::vector<unsigned char> data;
};

#endif
