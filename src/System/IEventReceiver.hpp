#ifndef PFFG_IEVENTRECEIVER_HDR
#define PFFG_IEVENTRECEIVER_HDR

struct IEvent;

class IEventReceiver {
public:
	IEventReceiver(): priority(0) {}
	virtual ~IEventReceiver() {}

	virtual bool WantsEvent(int) const { return false; }
	virtual void OnEvent(const IEvent*) = 0;

	void SetPriority(int p) { priority = p; }
	int GetPriority() const { return priority; }

private:
	int priority;
};

#endif
