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
	snprintf(s, 511, "[frame=%u][event=SimObjectMoveOrderEvent][goalPos=%s][numObjects=%u]", frame, (goalPos.str()).c_str(), objectIDs.size());
	return std::string(s);
}
