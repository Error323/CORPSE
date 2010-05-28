#ifndef PFFG_SIMOBJECTDEFLOADER_HDR
#define PFFG_SIMOBJECTDEFLOADER_HDR

#include <string>
#include <map>

class SimObjectDef;
class SimObjectDefLoader {
public:
	static void DelDefs();
	static SimObjectDef* GetDef(const std::string&);

private:
	static std::map<std::string, SimObjectDef*> objectDefs;
};

#endif

