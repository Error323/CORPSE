#ifndef PFFG_CCWIDGET_HDR
#define PFFG_CCWIDGET_HDR

#include "./UIWidget.hpp"
#include "../Math/vec3fwd.hpp"

#include <vector>
#include <string>

namespace ui {
	struct CCWidget: public IUIWidget {
	public:
		CCWidget() {}

		void Update(const vec3i&, const vec3i&);
	};
}

#endif
