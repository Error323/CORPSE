#include <GL/gl.h>
#include "./GLObjects.hpp"

namespace GL {
	void Light::Enable() { enabled = true; glEnable(GL_LIGHT0 + lightIdx); }
	void Light::Disable() { enabled = false; glDisable(GL_LIGHT0 + lightIdx); }
}
