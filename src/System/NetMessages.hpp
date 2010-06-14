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
	CLIENT_MSG_SIMFRAME    =  4,
	CLIENT_MSG_SIMCOMMAND  =  5,
};

// messages originating from server
enum ServerMessageIDs {
	SERVER_MSG_SIMFRAME = 1
};



struct NetMessage {
public:
	NetMessage(): messageID(-1), senderID(-1), pos(-1), full(true) {}
	NetMessage(unsigned int msgID, unsigned int sndID, unsigned int size): messageID(msgID), senderID(sndID), pos(0) {
		data.resize(size, 0); full = (size == 0);
	}
	~NetMessage() {
		data.clear();
	}

	NetMessage& operator = (const NetMessage& m) {
		// when copying, reset the position
		messageID = m.messageID;
		senderID  = m.senderID;
		pos       = 0;
		data      = m.data;
		full      = m.full;

		return *this;
	}

	template<typename T> NetMessage& operator << (T t) {
		assert(!full);
		assert((pos + sizeof(T)) <= data.size());
		memcpy(&data[pos], reinterpret_cast<unsigned char*>(&t), sizeof(T));
		pos += sizeof(T);
		full = (pos >= data.size());
		return *this;
	}
	template<typename T> NetMessage& operator << (const std::vector<T>& v) {
		for (typename std::vector<T>::const_iterator it = v.begin(); it != v.end(); ++it) {
			(*this) << (*it);
		}

		return *this;
	}

	template<typename T> NetMessage& operator >> (T& t) {
		assert(full);
		assert((pos + sizeof(T)) <= data.size());
		t = *(reinterpret_cast<T*>(&data[pos]));
		pos += sizeof(T);
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

	unsigned int GetMessageID() const { return messageID; }
	unsigned int GetSenderID() const { return senderID; }
	unsigned int GetPos() const { return pos; }
	void SetPos(unsigned int p) { pos = p; }
	bool End() const { return (pos >= data.size()); }

private:
	unsigned int messageID;
	unsigned int senderID;
	unsigned int pos;

	bool full;

	std::vector<unsigned char> data;
};

#endif
