#include "./ModelReaderBase.hpp"
#include "./ModelReader3DO.hpp"
/// #include "./ModelDrawer3DO.hpp"

CModelReader3DO* reader3DO = 0x0;

CModelReader3DO::CModelReader3DO() {
	/// drawer3DO = new CModelDrawer3DO();
}

CModelReader3DO::~CModelReader3DO() {
	/// delete drawer3DO; drawer3DO = 0x0;
}



ModelBase* CModelReader3DO::Load(const std::string& name) {
	ModelBase* model = new ModelBase();
	model->drawer = 0x0; /// drawer3DO;
	model->type = MODELTYPE_3DO;
	model->numObjects = 0;
	model->name = name;
	return model;
}
