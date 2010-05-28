#ifndef PFFG_SIMOBJECTDEF_HDR
#define PFFG_SIMOBJECTDEF_HDR

#include <string>

class SimObjectDef {
public:
	void SetModelName(const std::string& m) { modelName = m; }
	const std::string& GetModelName() const { return modelName; }

private:
	std::string modelName;
};

#endif
