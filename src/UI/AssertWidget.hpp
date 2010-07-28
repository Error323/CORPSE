#ifndef PFFG_ASSERT_HDR
#define PFFG_ASSERT_HDR

#include "./UIWidget.hpp"
#include "../Math/vec3fwd.hpp"

#include <vector>
#include <string>

namespace ui {
	struct AssertWidget: public IUIWidget {
	public:
		AssertWidget():
			mIsEnabled(false) {}
		void Update(const vec3i&, const vec3i&);
		void SetText(const char *msg) { mMessage += std::string(msg); }
		void Enable()  {mIsEnabled = true; mMessage.clear(); }
		void Disable() {mIsEnabled = false; mMessage.clear(); }
		static AssertWidget* GetInstance();

	private:
		bool mIsEnabled;
		std::string mMessage;
		static AssertWidget *mInstance;
	};
}

#endif
