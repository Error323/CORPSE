#ifndef PFFG_EVENTHANDLER_HDR
#define PFFG_EVENTHANDLER_HDR

#include <map>
#include <vector>

struct IEvent;
class IEventReceiver;

class EventHandler {
public:
	static EventHandler* GetInstance();
	static void FreeInstance(EventHandler*);

	void AddReceiver(IEventReceiver*);
	void DelReceiver(IEventReceiver*);
	void DelReceivers();

	void NotifyReceivers(const IEvent*) const;

private:
	EventHandler();
	~EventHandler();

	typedef std::map<int, IEventReceiver*> EvtReceiverMap;
	typedef std::map<int, IEventReceiver*>::const_iterator EvtReceiverMapIt;

	std::vector<EvtReceiverMap> evtReceivers;
};

#define eventHandler (EventHandler::GetInstance())

#endif
