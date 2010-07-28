#ifndef PFFG_ASSERTWIDGET_HDR
#define PFFG_ASSERTWIDGET_HDR

#include "./UIWidget.hpp"
#include "../Math/vec3fwd.hpp"

#include <vector>
#include <string>

namespace ui {
	struct AssertWidget: public IUIWidget {
	public:
		AssertWidget() {}

		void Update(const vec3i&, const vec3i&);
	};
}

#endif
