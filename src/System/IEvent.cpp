#include <cstdio>
#include <string>

#include "./IEvent.hpp"

std::string IEvent::str() const {
	snprintf(s, 511, "[frame=%u][event=EventBase]", frame);
	return std::string(s);
};



std::string SimObjectCreatedEvent::str() const {
	snprintf(s, 511, "[frame=%u][event=SimObjectCreated][objectID=%u]", frame, objectID);
	return std::string(s);
}

std::string SimObjectDestroyedEvent::str() const {
	snprintf(s, 511, "[frame=%u][event=SimObjectDestroyed][objectID=%u]", frame, objectID);
	return std::string(s);
}



std::string SimObjectMoveOrderEvent::str() const {
	snprintf(s, 511, "[frame=%u][event=SimObjectMoveOrderEvent][goalPos=%s][numObjects=%u][queued=%d]", frame, (goalPos.str()).c_str(), objectIDs.size(), queued);
	return std::string(s);
}

std::string SimObjectCollisionEvent::str() const {
	snprintf(s, 511, "[frame=%u][event=SimObjectCollisionEvent][colliderID=%u, collideeID=%u]", frame, colliderID, collideeID);
	return std::string(s);
}
