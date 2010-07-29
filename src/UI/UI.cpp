#include <SDL/SDL.h>

#include "./UI.hpp"
#include "./FontManager.hpp"
#include "./SimObjectSelectorWidget.hpp"
#include "./SimObjectSpawnerWidget.hpp"
#include "./HUDWidget.hpp"
#include "./AssertWidget.hpp"
#include "../Input/InputHandler.hpp"
#include "../System/EngineAux.hpp"
#include "../System/LuaParser.hpp"
#include "../System/Debugger.hpp"

ui::UI* ui::UI::GetInstance() {
	static UI* ui = NULL;
	static unsigned int depth = 0;

	if (ui == NULL) {
		PFFG_ASSERT(depth == 0);

		depth += 1;
		ui = new UI();
		depth -= 1;
	}

	return ui;
}

void ui::UI::FreeInstance(UI* ui) {
	delete ui;
}



ui::UI::UI() {
	mFontManager = IFontManager::GetInstance();

	widgets.push_back(new SimObjectSelectorWidget());
	widgets.push_back(new SimObjectSpawnerWidget());
	widgets.push_back(new HUDWidget());
	widgets.push_back(new AssertWidget());

	inputHandler->AddReceiver(this);

	const LuaTable* rootTable = LUA->GetRoot();
	const LuaTable* generalTable = rootTable->GetTblVal("general");
	const LuaTable* uiTable = rootTable->GetTblVal("ui");

	const std::string& fontsDir = generalTable->GetStrVal("fontsDir", "");
	const std::string& fontName = uiTable->GetStrVal("fontName", "");
	const int fontSize = uiTable->GetFltVal("fontSize", 72);

	mFont = mFontManager->GetFont(fontsDir + fontName, fontSize);
}

ui::UI::~UI() {
	inputHandler->DelReceiver(this);

	for (std::list<IUIWidget*>::iterator it = widgets.begin(); it != widgets.end(); ++it) {
		delete *it;
	}

	IFontManager::FreeInstance(mFontManager);
}

void ui::UI::Update(const vec3i& pos, const vec3i& size) {
	for (std::list<IUIWidget*>::iterator it = widgets.begin(); it != widgets.end(); ++it) {
		(*it)->Update(pos, size);
	}
}



void ui::UI::KeyPressed(int key, bool repeat) {
	if (!repeat) {
		for (std::list<IUIWidget*>::iterator it = widgets.begin(); it != widgets.end(); ++it) {
			(*it)->KeyPressed(key);
		}
	}
}
void ui::UI::KeyReleased(int key) {
	for (std::list<IUIWidget*>::iterator it = widgets.begin(); it != widgets.end(); ++it) {
		(*it)->KeyReleased(key);
	}
}

void ui::UI::MouseMoved(int x, int y, int dx, int dy) {
	for (std::list<IUIWidget*>::iterator it = widgets.begin(); it != widgets.end(); ++it) {
		(*it)->MouseMoved(x, y, dx, dy);
	}
}

void ui::UI::MousePressed(int button, int x, int y, bool repeat) {
	if (!repeat) {
		for (std::list<IUIWidget*>::iterator it = widgets.begin(); it != widgets.end(); ++it) {
			(*it)->MousePressed(button, x, y);
		}
	}
}

void ui::UI::MouseReleased(int button, int x, int y) {
	for (std::list<IUIWidget*>::iterator it = widgets.begin(); it != widgets.end(); ++it) {
		(*it)->MouseReleased(button, x, y);
	}
}
