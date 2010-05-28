#ifndef PFFG_SIMOBJECTHANDLER_HDR
#define PFFG_SIMOBJECTHANDLER_HDR

#include <list>

class SimObject;
class SimObjectHandler {
public:
	SimObjectHandler();
	~SimObjectHandler();

	static SimObjectHandler* GetInstance();
	static void FreeInstance(SimObjectHandler*);

	void Update();

	const std::list<SimObject*>& GetSimObjects() const { return simObjects; }

private:
	std::list<SimObject*> simObjects;
};

#endif
