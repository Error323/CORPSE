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

	void AddObject(SimObject*);
	void DelObject(SimObject*);

	const std::set<unsigned int>& GetSimObjectFreeIDs() const { return simObjectFreeIDs; }
	const std::set<unsigned int>& GetSimObjectUsedIDs() const { return simObjectUsedIDs; }

	unsigned int GetNumSimObjects() const { return simObjectUsedIDs.size(); }
	unsigned int GetMaxSimObjects() const { return simObjects.size(); }
	SimObject* GetSimObject(unsigned int id) const { return simObjects[id]; }

private:
	std::vector<SimObject*> simObjects;
	std::set<unsigned int> simObjectFreeIDs;
	std::set<unsigned int> simObjectUsedIDs;
};

#define simObjectHandler (SimObjectHandler::GetInstance())

#endif
