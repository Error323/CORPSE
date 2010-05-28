#ifndef PFFG_SIMOBJECTHANDLER_HDR
#define PFFG_SIMOBJECTHANDLER_HDR

#include <set>
#include <vector>

class SimObject;
class SimObjectHandler {
public:
	SimObjectHandler();
	~SimObjectHandler();

	static SimObjectHandler* GetInstance();
	static void FreeInstance(SimObjectHandler*);

	void Update();

	const std::set<unsigned int>& GetSimObjectFreeIDs() const { return simObjectFreeIDs; }
	const std::set<unsigned int>& GetSimObjectUsedIDs() const { return simObjectUsedIDs; }

	SimObject* GetSimObject(unsigned int id) const { return simObjects[id]; }

private:
	std::vector<SimObject*> simObjects;
	std::set<unsigned int> simObjectFreeIDs;
	std::set<unsigned int> simObjectUsedIDs;
};

#endif
