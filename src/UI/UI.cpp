#include <cassert>

#include <SDL/SDL.h>

#include "./UI.hpp"
#include "./FontManager.hpp"
#include "./HUD.hpp"
#include "./SimObjectSelector.hpp"
#include "./SimObjectSpawner.hpp"
#include "../Input/InputHandler.hpp"
#include "../System/EngineAux.hpp"
#include "../System/LuaParser.hpp"

ui::UI* ui::UI::GetInstance() {
	static UI* ui = NULL;
	static unsigned int depth = 0;

	if (ui == NULL) {
		assert(depth == 0);

		depth += 1;
		ui = new UI();
		depth -= 1;
	}

	return ui;
}

void ui::UI::FreeInstance(UI* ui) {
	delete ui;
}



ui::UI::UI(): mSimObjectSelector(NULL) {
	mFontManager = IFontManager::GetInstance();
	mSimObjectSelector = new SimObjectSelector();
	mSimObjectSpawner = new SimObjectSpawner();
	mHUD = new HUD();

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
	delete mSimObjectSelector;
	delete mSimObjectSpawner;
	delete mHUD;
	IFontManager::FreeInstance(mFontManager);
}

void ui::UI::Update(const vec3i& pos, const vec3i& size) {
	mSimObjectSelector->Update();
	mSimObjectSpawner->Update();
	mHUD->Update(pos, size);
}



void ui::UI::MouseMoved(int x, int y, int dx, int dy) {
	mSimObjectSelector->MouseMoved(x, y, dx, dy);
	mSimObjectSpawner->MouseMoved(x, y, dx, dy);
}

void ui::UI::MousePressed(int button, int x, int y, bool repeat) {
	if (!repeat) {
		mSimObjectSelector->MousePressed(button, x, y);
		mSimObjectSpawner->MousePressed(button, x, y);
	}
}

void ui::UI::MouseReleased(int button, int x, int y) {
	mSimObjectSelector->MouseReleased(button, x, y);
	mSimObjectSpawner->MouseReleased(button, x, y);
}
