#include <cstdio>
#include <string>

#include "./IEvent.hpp"

std::string IEvent::str() const {
	snprintf(s, 511, "[frame=%d][event=EventBase]", frame);
	return std::string(s);
};



std::string SimObjectCreatedEvent::str() const {
	snprintf(s, 511, "[frame=%d][event=SimObjectCreated][objectID=%d]", frame, objectID);
	return std::string(s);
}

std::string SimObjectDestroyedEvent::str() const {
	snprintf(s, 511, "[frame=%d][event=SimObjectDestroyed][objectID=%d]", frame, objectID);
	return std::string(s);
}
