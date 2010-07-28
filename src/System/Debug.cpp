#include "./Debug.hpp"
#include "../UI/AssertWidget.hpp"
#include "../UI/Window.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>

Debug *Debug::mInstance = NULL;
Debug *theDebugger = Debug::GetInstance();

Debug* Debug::GetInstance() {
	if (mInstance == NULL)
		mInstance = new Debug();
	return mInstance;
}

void Debug::Init() {
	mInputHandler = CInputHandler::GetInstance();
	mInputHandler->AddReceiver(this);
}

Debug::~Debug() {
	delete mKey;
}

void Debug::Pause() {
}

void Debug::Begin(const char *filename, int line) {
	sprintf(mKey, "%s:%d", filename, line);

	std::map<char*, bool>::iterator i;
	i = mIgnoreForever.find(mKey);
	if (i == mIgnoreForever.end()) {
		mIgnoreForever[mKey] = false;
	}

	if (mIgnoreForever[mKey])
		return;

	using namespace ui;
	AssertWidget *assertWidget = AssertWidget::GetInstance();
	if (assertWidget != NULL)
		assertWidget->Enable();
}

void Debug::Print(const char *msg) {
	if (mIgnoreForever[mKey])
		return;

	using namespace ui;
	AssertWidget *assertWidget = AssertWidget::GetInstance();
	if (assertWidget != NULL)
		assertWidget->SetText(msg);
	fprintf(stderr, msg);
}

bool Debug::End() {
	bool breakPoint = false;

	if (mIgnoreForever[mKey])
		return breakPoint;

	Print("\n\nPress LEFT for debugging, UP for ignore or DOWN for ignore forever");
	using namespace ui;
	IWindow* window = IWindow::GetInstance();
	if (window != NULL)
		window->Update();

	// handle input queries
	EnableInput();
	bool input = true;
	while (input) {
		switch(mKeyReleased) {
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
	
	AssertWidget *assertWidget = AssertWidget::GetInstance();
	if (assertWidget != NULL)
		assertWidget->Disable();

	return breakPoint;
}

void Debug::KeyReleased(int key) {
	mKeyReleased = key;
}
