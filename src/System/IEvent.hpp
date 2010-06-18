#ifndef PFFG_IEVENT_HDR
#define PFFG_IEVENT_HDR

#include <list>

#include "../Math/vec3fwd.hpp"
#include "../Math/vec3.hpp"

enum EventType {
	EVENT_BASE                = -1,
	EVENT_SIMOBJECT_CREATED   =  0,
	EVENT_SIMOBJECT_DESTROYED =  1,
	EVENT_SIMOBJECT_MOVEORDER =  2,
	EVENT_SIMOBJECT_COLLISION =  3,
	EVENT_LAST                =  4,
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
	SimObjectCreatedEvent(unsigned int f, unsigned int objID): IEvent(EVENT_SIMOBJECT_CREATED, f) {
		objectID = objID;
	}

	unsigned int GetObjectID() const { return objectID; }
	std::string str() const;

private:
	unsigned int objectID;
};

struct SimObjectDestroyedEvent: public IEvent {
public:
	SimObjectDestroyedEvent(unsigned int f, unsigned int objID): IEvent(EVENT_SIMOBJECT_DESTROYED, f) {
		objectID = objID;
	}

	unsigned int GetObjectID() const { return objectID; }
	std::string str() const;

private:
	unsigned int objectID;
};



struct SimObjectMoveOrderEvent: public IEvent {
public:
	SimObjectMoveOrderEvent(unsigned int f): IEvent(EVENT_SIMOBJECT_MOVEORDER, f) {
	}

	void AddObjectID(unsigned int id) { objectIDs.push_back(id); }
	const std::list<unsigned int>& GetObjectIDs() const { return objectIDs; }

	void SetGoalPos(const vec3f& pos) { goalPos = pos; }
	const vec3f& GetGoalPos() const { return goalPos; }

	std::string str() const;

private:
	// sim-objects that received the move-order to <goalPos>
	// NOTE: transferred across DLL boundary, so not ABI-safe
	std::list<unsigned int> objectIDs;

	// shared destination of all involved sim-objects
	vec3f goalPos;
};



struct SimObjectCollisionEvent: public IEvent {
public:
	SimObjectCollisionEvent(unsigned int f, unsigned int a0, unsigned int a1): IEvent(EVENT_SIMOBJECT_COLLISION, f) {
		colliderID = a0;
		collideeID = a1;
	}

	unsigned int GetColliderID() const { return colliderID; }
	unsigned int GetCollideeID() const { return collideeID; }

	std::string str() const;

private:
	unsigned int colliderID;
	unsigned int collideeID;
};

#endif
