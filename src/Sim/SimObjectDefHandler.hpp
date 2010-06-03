#ifndef PFFG_SIMOBJECTDEF_HANDLER_HDR
#define PFFG_SIMOBJECTDEF_HANDLER_HDR

#include <string>
#include <map>

class SimObjectDef;
class SimObjectDefHandler {
public:
	static SimObjectDefHandler* GetInstance();
	static void FreeInstance(SimObjectDefHandler*);

	bool LoadDefs();
	void DelDefs();

	SimObjectDef* GetDef(const std::string&);

private:
	std::map<std::string, SimObjectDef*> objectDefs;
};

#endif

