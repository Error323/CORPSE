#include "./Debugger.hpp"

#include "../Input/InputHandler.hpp"
#include "../UI/Window.hpp"

#include <SDL.h>
#include <stdio.h>

Debugger* Debugger::GetInstance() {
	static Debugger* d = NULL;

	if (d == NULL) {
		d = new Debugger();
	}

	return d;
}

void Debugger::FreeInstance(Debugger* d) {
	delete d;
}



#ifdef PFFG_DEBUG
Debugger::Debugger(): mEnabled(false) {
	mInputHandler = CInputHandler::GetInstance();
	mInputHandler->AddReceiver(this);
}

Debugger::~Debugger() {
	mInputHandler->DelReceiver(this);
}

bool Debugger::Begin(const char* filename, int line) {
	snprintf(gDebugMessageKey, 1024, "%s:%d", filename, line);

	mKey = std::string(gDebugMessageKey);
	std::map<std::string, bool>::iterator i = mIgnoreForever.find(mKey);

	if (i == mIgnoreForever.end())
		mIgnoreForever[mKey] = false;

	// If we are ignored don't start
	if (mIgnoreForever[mKey])
		return false;

	mEnabled = true;
	mMessage.clear();

	return true;
}

void Debugger::Print(const char* msg) {
	mMessage += std::string(msg);
	fprintf(stderr, msg);
}

bool Debugger::End() {
	bool breakPoint = false;

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

void Debugger::DumpStack(char **symbols, int size) {
	Debugger::GetInstance()->Print("\n");
	if (symbols != NULL) {
		for (size_t i = 0; i < size; i++) {
			Debugger::GetInstance()->Print(symbols[i]);
			Debugger::GetInstance()->Print("\n");
		}
	}
}
#endif // PFFG_DEBUG
