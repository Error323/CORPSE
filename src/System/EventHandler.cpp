#include <cassert>
#include <string>

#include "./EventHandler.hpp"
#include "./IEventReceiver.hpp"
#include "./IEvent.hpp"

EventHandler* EventHandler::GetInstance() {
	static EventHandler* eh = NULL;
	static unsigned int depth = 0;

	if (eh == NULL) {
		assert(depth == 0);

		depth += 1;
		eh = new EventHandler();
		depth -= 1;
	}

	return eh;
}

void EventHandler::FreeInstance(EventHandler* eh) {
	delete eh;
}



EventHandler::EventHandler() { evtReceivers.resize(EVENT_LAST); }
EventHandler::~EventHandler() { evtReceivers.clear(); }

void EventHandler::AddReceiver(IEventReceiver* r) {
	for (int type = int(EVENT_BASE); type < EVENT_LAST; type++) {
		if (!r->WantsEvent(type)) {
			continue;
		}

		if (evtReceivers[type].find(r->GetPriority()) == evtReceivers[type].end()) {
			evtReceivers[type][r->GetPriority()] = r;
		}
	}
}

void EventHandler::DelReceiver(IEventReceiver* r) {
	for (int type = int(EVENT_BASE); type < EVENT_LAST; type++) {
		if (!r->WantsEvent(type)) {
			continue;
		}

		if (evtReceivers[type].find(r->GetPriority()) != evtReceivers[type].end()) {
			evtReceivers[type].erase(r->GetPriority());
		}
	}
}

void EventHandler::DelReceivers() {
	for (int type = int(EVENT_BASE); type < EVENT_LAST; type++) {
		evtReceivers[type].clear();
	}
}

void EventHandler::NotifyReceivers(const IEvent* e) const {
	for (EvtReceiverMapIt it = evtReceivers[e->GetType()].begin(); it != evtReceivers[e->GetType()].end(); ++it) {
		it->second->OnEvent(e);
	}
}
