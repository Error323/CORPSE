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
	mSimObjectSelector->Update();
	mSimObjectSpawner->Update();
}



void UI::MouseMoved(int x, int y, int dx, int dy) {
	mSimObjectSelector->MouseMoved(x, y, dx, dy);
	mSimObjectSpawner->MouseMoved(x, y, dx, dy);
}

void UI::MousePressed(int button, int x, int y, bool repeat) {
	if (!repeat) {
		mSimObjectSelector->MousePressed(button, x, y);
		mSimObjectSpawner->MousePressed(button, x, y);
	}
}

void UI::MouseReleased(int button, int x, int y) {
	mSimObjectSelector->MouseReleased(button, x, y);
	mSimObjectSpawner->MouseReleased(button, x, y);
}
