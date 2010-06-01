#ifndef PFFG_NETMESSAGES_HDR
#define PFFG_NETMESSAGES_HDR

enum ClientMessages {
	CLIENT_MSG_NONE        = -1,
	CLIENT_MSG_PAUSE       =  0,
	CLIENT_MSG_INCSIMSPEED =  1,
	CLIENT_MSG_DECSIMSPEED =  2,
};

enum ServerMessages {
	SERVER_MSG_NONE     = -1,
	SERVER_MSG_SIMFRAME =  0
};

#endif
