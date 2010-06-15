#ifndef PFFG_UI_HDR
#define PFFG_UI_HDR

#include <list>

#include "../Input/InputReceiver.hpp"
#include "../Math/vec3fwd.hpp"

class FTFont;

namespace ui {
	class IFontManager;
	struct IUIWidget;

	class UI: public CInputReceiver {
	public:
		static UI* GetInstance();
		static void FreeInstance(UI*);

		void Update(const vec3i&, const vec3i&);

		void MouseMoved(int, int, int, int);
		void MousePressed(int, int, int, bool);
		void MouseReleased(int, int, int);

		FTFont* GetFont() const { return mFont; }

	private:
		UI();
		~UI();

		IFontManager* mFontManager;
		FTFont* mFont;

		std::list<IUIWidget*> widgets;
	};
}

#define gUI (ui::UI::GetInstance())

#endif
