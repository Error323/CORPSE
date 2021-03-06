#ifndef PFFG_SIMOBJECTHANDLER_HDR
#define PFFG_SIMOBJECTHANDLER_HDR

#include <map>
#include <set>
#include <list>
#include <vector>

#include "../Math/vec3fwd.hpp"

class SimObject;
class SimObjectDefHandler;
template<typename T> class SimObjectGrid;

class SimObjectHandler {
public:
	SimObjectHandler();
	~SimObjectHandler();

	static SimObjectHandler* GetInstance();
	static void FreeInstance(SimObjectHandler*);

	void Update(unsigned int);

	void AddObjects();
	void DelObjects();
	void AddObject(unsigned int, unsigned int, const vec3f&, const vec3f&, bool = false);
	void DelObject(unsigned int, bool = false);

	const std::set<unsigned int>& GetSimObjectFreeIDs() const { return simObjectFreeIDs; }
	const std::set<unsigned int>& GetSimObjectUsedIDs() const { return simObjectUsedIDs; }

	unsigned int GetNumSimObjects() const { return simObjectUsedIDs.size(); }
	unsigned int GetMaxSimObjects() const { return simObjects.size(); }
	bool IsValidSimObjectID(unsigned int id) const { return ((id < GetMaxSimObjects()) && (simObjects[id] != NULL)); }

	SimObject* GetSimObject(unsigned int id) const { return simObjects[id]; }
	SimObjectGrid<const SimObject*>* GetSimObjectGrid() const { return mSimObjectGrid; }

	const SimObject* GetClosestSimObject(const vec3f&, float) const;

private:
	void AddObject(SimObject*, bool);
	void DelObject(SimObject*, bool);

	unsigned int CheckSimObjectCollisions(unsigned int);
	void PredictSimObjectCollisions(unsigned int);

	std::vector<SimObject*> simObjects;
	// for each object, keep track of the cells it occupies
	// objectID: {cell index ==> cell object-list iterator}
	std::vector<  std::map<unsigned int, std::list<const SimObject*>::iterator>  > simObjectGridCells;

	std::set<unsigned int> simObjectFreeIDs;
	std::set<unsigned int> simObjectUsedIDs;

	SimObjectDefHandler* mSimObjectDefHandler;
	SimObjectGrid<const SimObject*>* mSimObjectGrid;
};

#define simObjectHandler (SimObjectHandler::GetInstance())

#endif
