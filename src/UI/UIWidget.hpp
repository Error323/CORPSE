#ifndef PFFG_UIWIDGET_HDR
#define PFFG_UIWIDGET_HDR

#include "../Math/vec3fwd.hpp"

namespace ui {
	struct IUIWidget {
		virtual ~IUIWidget() {}
		virtual void Update(const vec3i&, const vec3i&) {}
		virtual void KeyPressed(int) {}
		virtual void KeyReleased(int) {}
		virtual void MousePressed(int, int, int) {}
		virtual void MouseReleased(int, int, int) {}
		virtual void MouseMoved(int, int, int, int) {}
	};
}

#endif
