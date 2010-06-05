#ifndef PFFG_SIMCOMMANDS_HDR
#define PFFG_SIMCOMMANDS_HDR

enum CommandType {
	COMMAND_CREATE_SIMOBJECT  = 0, // uint objectDefID, vec3f objectPos
	COMMAND_DESTROY_SIMOBJECT = 1, // uint objectID (1)
	COMMAND_MOVE_SIMOBJECT    = 2, // vec3f goalPos (1), uint objectID (>= 1)
	COMMAND_LAST              = 3,
};

#endif
