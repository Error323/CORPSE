#ifndef PFFG_SIMOBJECTDEF_HANDLER_HDR
#define PFFG_SIMOBJECTDEF_HANDLER_HDR

#include <string>
#include <map>
#include <vector>

class SimObjectDef;
class SimObjectDefHandler {
public:
	static SimObjectDefHandler* GetInstance();
	static void FreeInstance(SimObjectDefHandler*);

	bool LoadDefs();
	void DelDefs();

	unsigned int GetNumDefs() const { return objectDefsVec.size(); }

	SimObjectDef* GetDef(const std::string&);
	SimObjectDef* GetDef(unsigned int);

private:
	std::map<std::string, SimObjectDef*> objectDefsMap;
	std::vector<SimObjectDef*> objectDefsVec;
};

#define simObjectDefHandler (SimObjectDefHandler::GetInstance())

#endif
