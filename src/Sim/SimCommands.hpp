#ifndef PFFG_SIMCOMMANDS_HDR
#define PFFG_SIMCOMMANDS_HDR

enum CommandType {
	COMMAND_CREATE_SIMOBJECT  = 0, // objectDefID, objectPos
	COMMAND_DESTROY_SIMOBJECT = 1, // objectID (1)
	COMMAND_MOVE_SIMOBJECT    = 2, // goalPos (1), objectID (>= 1)
	COMMAND_LAST              = 3,
};

#endif
