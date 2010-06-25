#include "./SimObject.hpp"
#include "./SimObjectDef.hpp"

void SimObject::Update() {
	PhysicalState p = physicalState;
		p.Update(this);
	physicalState = p;
}
