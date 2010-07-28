#include "./Debugger.hpp"
#include "../UI/Window.hpp"

#include <cstdio>
#include <cstdlib>
#include <SDL.h>

Debugger* Debugger::GetInstance() {
	static Debugger* instance = NULL;

	if (instance == NULL) {
		instance = new Debugger();
	}

	return instance;
}

void Debugger::FreeInstance(Debugger* d) {
	delete d;
}



Debugger::Debugger(): mEnabled(false) {
	mInputHandler = CInputHandler::GetInstance();
	mInputHandler->AddReceiver(this);
}

Debugger::~Debugger() {
	mInputHandler->DelReceiver(this);
	delete mKey;
}

void Debugger::Begin(const char* filename, int line) {
	sprintf(mKey, "%s:%d", filename, line);

	std::map<char*, bool>::iterator i = mIgnoreForever.find(mKey);
	if (i == mIgnoreForever.end()) {
		mIgnoreForever[mKey] = false;
	}

	if (mIgnoreForever[mKey])
		return;

	mEnabled = true;
	mMessage.clear();
}

void Debugger::Print(const char* msg) {
	if (mIgnoreForever[mKey])
		return;

	mMessage += std::string(msg);
	fprintf(stderr, msg);
}

bool Debugger::End() {
	bool breakPoint = false;

	if (mIgnoreForever[mKey])
		return breakPoint;

	Print("\n\nPress LEFT for debugging, UP for ignore or DOWN for ignore forever");
	/// FIXME
	using namespace ui;
	IWindow* window = IWindow::GetInstance();
	if (window != NULL)
		window->Update();

	// handle input queries
	EnableInput();
	bool input = true;
	while (input) {
		switch (mKeyReleased) {
			case SDLK_LEFT: 
				input = false;
				breakPoint = true;
				break;

			case SDLK_UP:
				input = false;
				break;

			case SDLK_DOWN: 
				mIgnoreForever[mKey] = true; 
				input = false;
				break;

			default: break;
		}
		mInputHandler->Update();
	}
	mKeyReleased = 0;
	DisableInput();

	mEnabled = false;
	mMessage.clear();

	return breakPoint;
}

void Debugger::KeyReleased(int key) {
	mKeyReleased = key;
}
