#include "./Debugger.hpp"

#include "../Input/InputHandler.hpp"
#include "../UI/Window.hpp"

#include <SDL.h>

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

void Debugger::Begin(const char* filename, int line) {
	snprintf(gDebugMessageKey, 1024, "%s:%d", filename, line);

	mKey = std::string(gDebugMessageKey);
	std::map<std::string, bool>::iterator i = mIgnoreForever.find(mKey);

	if (i == mIgnoreForever.end())
		mIgnoreForever[mKey] = false;

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

void Debugger::DumpStack() {
	#if (!defined(WIN32) && !defined(__powerpc64__))
	void* addresses[16];

	size_t size = backtrace(addresses, 16);
	char** symbols = backtrace_symbols(addresses, size);

	if (symbols != NULL) {
		for (size_t i = 0; i < size; i++) {
			Debugger::GetInstance()->Print(symbols[i]);
			Debugger::GetInstance()->Print("\n");
		}

		free(symbols);
	}

	#endif
}
#endif // PFFG_DEBUG
