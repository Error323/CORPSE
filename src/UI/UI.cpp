#include <cassert>

#include <SDL/SDL.h>

#include "./UI.hpp"
#include "./SimObjectSelector.hpp"
#include "../Input/InputHandler.hpp"

UI* UI::GetInstance() {
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

void UI::FreeInstance(UI* ui) {
	delete ui;
}



UI::UI(): mSimObjectSelector(NULL) {
	mSimObjectSelector = new SimObjectSelector();
	inputHandler->AddReceiver(this);
}

UI::~UI() {
	inputHandler->DelReceiver(this);
	delete mSimObjectSelector;
}

void UI::Update() {
	mSimObjectSelector->DrawSelection();
}



void UI::MouseMoved(int x, int y, int, int) {
	mSimObjectSelector->UpdateSelection(x, y);
}

void UI::MousePressed(int button, int x, int y, bool repeat) {
	if (button == SDL_BUTTON_LEFT && !repeat) {
		mSimObjectSelector->StartSelection(x, y);
	}
}

void UI::MouseReleased(int button, int x, int y) {
	switch (button) {
		case SDL_BUTTON_LEFT: {
			mSimObjectSelector->FinishSelection(x, y);
		} break;
		case SDL_BUTTON_RIGHT: {
			mSimObjectSelector->GiveSelectionOrder(x, y);
		}
	}
}
