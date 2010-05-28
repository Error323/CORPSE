#include "./SimObject.hpp"
#include "./SimObjectDef.hpp"
#include "../Renderer/Models/ModelReaderBase.hpp"

SimObject::~SimObject() {
	// need to clean this up elsewhere
	// (possibly have the renderer wait
	// for SimObjectDestroyed events?)
	delete mdl;
	mdl = 0;
	def = 0;
}

void SimObject::Update() {
}
