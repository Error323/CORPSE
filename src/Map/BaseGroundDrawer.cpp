#include <GL/gl.h>

#include "./BaseGroundDrawer.hpp"
#include "./Ground.hpp"
//// #include "./HeightLinePalette.hpp"
#include "./ReadMap.hpp"

CBaseGroundDrawer::CBaseGroundDrawer(void) {
	wireframe = false;
}

CBaseGroundDrawer::~CBaseGroundDrawer(void) {
}
