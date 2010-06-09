#include <cassert>

#include <SDL/SDL.h>

#include "./UI.hpp"
#include "./SimObjectSelector.hpp"
#include "./SimObjectSpawner.hpp"
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
	mSimObjectSpawner = new SimObjectSpawner();
	inputHandler->AddReceiver(this);
}

UI::~UI() {
	inputHandler->DelReceiver(this);
	delete mSimObjectSelector;
	delete mSimObjectSpawner;
}

void UI::Update() {
	mSimObjectSelector->DrawSelection();
}



void UI::MouseMoved(int x, int y, int, int) {
	mSimObjectSelector->UpdateSelection(x, y);
}

void UI::MousePressed(int button, int x, int y, bool repeat) {
	switch (button) {
		case SDL_BUTTON_LEFT: {
			if (!repeat) {
				mSimObjectSelector->StartSelection(x, y);
			}
		} break;
	}
}

void UI::MouseReleased(int button, int x, int y) {
	switch (button) {
		case SDL_BUTTON_LEFT: {
			mSimObjectSelector->FinishSelection(x, y);
		} break;
		case SDL_BUTTON_MIDDLE: {
			mSimObjectSpawner->SpawnObject(x, y);
		} break;
		case SDL_BUTTON_RIGHT: {
			mSimObjectSelector->GiveSelectionOrder(x, y);
		}
	}
}
