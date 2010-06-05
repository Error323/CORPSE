#ifndef PFFG_SIMOBJECTHANDLER_HDR
#define PFFG_SIMOBJECTHANDLER_HDR

#include <set>
#include <vector>

class SimObject;
class SimObjectDefHandler;
class SimObjectHandler {
public:
	SimObjectHandler();
	~SimObjectHandler();

	static SimObjectHandler* GetInstance();
	static void FreeInstance(SimObjectHandler*);

	void Update();

	void AddObjects();
	void DelObjects();
	void AddObject(SimObject*, bool);
	void DelObject(SimObject*, bool);

	const std::set<unsigned int>& GetSimObjectFreeIDs() const { return simObjectFreeIDs; }
	const std::set<unsigned int>& GetSimObjectUsedIDs() const { return simObjectUsedIDs; }

	unsigned int GetNumSimObjects() const { return simObjectUsedIDs.size(); }
	unsigned int GetMaxSimObjects() const { return simObjects.size(); }

	SimObject* GetSimObject(unsigned int id) const { return simObjects[id]; }

private:
	std::vector<SimObject*> simObjects;
	std::set<unsigned int> simObjectFreeIDs;
	std::set<unsigned int> simObjectUsedIDs;

	SimObjectDefHandler* mSimObjectDefHandler;
};

#define simObjectHandler (SimObjectHandler::GetInstance())

#endif
