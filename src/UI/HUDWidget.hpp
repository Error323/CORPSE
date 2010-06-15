#ifndef PFFG_HUD_HDR
#define PFFG_HUD_HDR

#include "./UIWidget.hpp"
#include "../Math/vec3fwd.hpp"

namespace ui {
	struct HUDWidget: public IUIWidget {
	public:
		void Update(const vec3i&, const vec3i&);
	};
}

#endif
