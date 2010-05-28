#ifndef PFFG_SIMOBJECT_HDR
#define PFFG_SIMOBJECT_HDR

#include <string>

#include "../Math/mat44fwd.hpp"
#include "../Math/mat44.hpp"

struct LocalModel;
class SimObjectDef;
class SimObject {
public:
	SimObject(SimObjectDef* d): def(d) {}
	virtual ~SimObject();
	virtual void Update();

	const SimObjectDef* GetDef() const { return def; }

	void SetModel(LocalModel* m) { mdl = m; }
	LocalModel* GetModel() const { return mdl; }

	const mat44f& GetMat() const { return mat; }
	void SetMat(const mat44f& m) { mat = m; }

private:
	const SimObjectDef* def;

	std::string mdlName;
	LocalModel* mdl;
	mat44f mat;
};

#endif
