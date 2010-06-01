#ifndef PFFG_IEVENT_HDR
#define PFFG_IEVENT_HDR

#include "../Math/vec3fwd.hpp"

enum EventType {
	EVENT_BASE                = -1,
	EVENT_SIMOBJECT_CREATED   =  0,
	EVENT_SIMOBJECT_DESTROYED =  2,
	EVENT_LAST                =  3,
};

struct IEvent {
public:
	IEvent(EventType t, unsigned int f): type(t), frame(f) {}
	virtual ~IEvent() {}

	EventType GetType() const { return type; }
	virtual std::string str() const;

protected:
	EventType type;
	unsigned int frame;

	mutable char s[512];
};



struct SimObjectCreatedEvent: public IEvent {
public:
	SimObjectCreatedEvent(unsigned int f, unsigned int a0): IEvent(EVENT_SIMOBJECT_CREATED, f) {
		objectID = a0;
	}

	unsigned int GetObjectID() const { return objectID; }
	std::string str() const;

private:
	unsigned int objectID;
};

struct SimObjectDestroyedEvent: public IEvent {
public:
	SimObjectDestroyedEvent(unsigned int f, unsigned int a0): IEvent(EVENT_SIMOBJECT_DESTROYED, f) {
		objectID = a0;
	}

	unsigned int GetObjectID() const { return objectID; }
	std::string str() const;

private:
	unsigned int objectID;
};

#endif
