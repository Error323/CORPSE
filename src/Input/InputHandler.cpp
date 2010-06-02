#include <SDL/SDL.h>

#include "../System/EngineAux.hpp"
#include "./InputHandler.hpp"
#include "./InputReceiver.hpp"

CInputHandler* CInputHandler::GetInstance() {
	static CInputHandler* ih = NULL;
	static unsigned int depth = 0;

	if (ih == NULL) {
		assert(depth == 0);

		depth += 1;
		ih = new CInputHandler();
		depth -= 1;
	}

	return ih;
}

void CInputHandler::FreeInstance(CInputHandler* ih) {
	delete ih;
}



CInputHandler::CInputHandler() {
	keys.resize(SDLK_LAST, 0);

	// X----------X--X--X--X--X--X--X
	// SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY >> 2, SDL_DEFAULT_REPEAT_INTERVAL);
	SDL_EnableUNICODE(1);
	SDL_SetModState(KMOD_NONE);
	SDL_ShowCursor(ENG->GetMouseLook()? SDL_DISABLE: SDL_ENABLE);
	SDL_WM_GrabInput(ENG->GetMouseLook()? SDL_GRAB_ON: SDL_GRAB_OFF);
}

CInputHandler::~CInputHandler() {
	SDL_WM_GrabInput(SDL_GRAB_OFF);

	keys.clear();
}



void CInputHandler::AddReceiver(CInputReceiver* ir) {
	if (inputReceivers.find(ir) != inputReceivers.end()) {
		return;
	}

	inputReceivers.insert(ir);
}
void CInputHandler::DelReceiver(CInputReceiver* ir) {
	if (inputReceivers.find(ir) == inputReceivers.end()) {
		return;
	}

	inputReceivers.erase(ir);
}



void CInputHandler::Update() {
	if ((SDL_GetTicks() - INP->GetLastInputTick()) >= INP->GetInputFrameTime()) {
		INP->SetLastInputTick(SDL_GetTicks());
	} else {
		return;
	}

	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_VIDEORESIZE: {
				WindowResized(&event);
			} break;

			case SDL_VIDEOEXPOSE: {
				WindowExposed(&event);
			} break;

			case SDL_ACTIVEEVENT: {
				WindowResized(&event);
			} break;


			case SDL_QUIT: {
				// closing via the WM
				ENG->SetWantQuit(true);
			} break;
			case SDL_SYSWMEVENT: {
				// unhandled event received from WM
			} break;


			case SDL_MOUSEMOTION: {
				MouseMoved(&event);
			} break;
			case SDL_MOUSEBUTTONDOWN: {
				MousePressed(&event);
			} break;
			case SDL_MOUSEBUTTONUP: {
				MouseReleased(&event);
			} break;

			case SDL_KEYDOWN: {
				KeyPressed(&event);
			} break;
			case SDL_KEYUP: {
				KeyReleased(&event);
			} break;
		}
	}

	UpdateKeyState();
	UpdateMouseState();
}



// refresh the keyboard array
void CInputHandler::UpdateKeyState() {
	int numKeys = 0;
	const Uint8* state = SDL_GetKeyState(&numKeys);
	const SDLMod mods = SDL_GetModState();

	memcpy(&keys[0], state, sizeof(Uint8) * numKeys);

	keys[SDLK_LALT]   = (mods & KMOD_ALT)  ? 1: 0;
	keys[SDLK_LCTRL]  = (mods & KMOD_CTRL) ? 1: 0;
	keys[SDLK_LMETA]  = (mods & KMOD_META) ? 1: 0;
	keys[SDLK_LSHIFT] = (mods & KMOD_SHIFT)? 1: 0;

	// note: excessive? <numKeys> is always equal to
	// SDLK_LAST no matter how many keys are pressed
	for (int i = numKeys - 1; i >= 0; i--) {
		if (keys[i]) {
			for (ReceiverIt it = inputReceivers.begin(); it != inputReceivers.end(); it++) {
				if ((*it)->InputEnabled()) {
					(*it)->KeyPressed(i, true);
				}
			}
		}
	}
}

void CInputHandler::UpdateMouseState() {
	int mx = 0;
	int my = 0;
	Uint8 state = SDL_GetMouseState(&mx, &my);

	INP->SetCurrMouseCoors(mx, my);

	if (state == 0) {
		// if (state & SDL_BUTTON_{LMASK, MMASK, RMASK})) {}
		INP->SetLastMouseButton(-1);
	}

	if (INP->GetLastMouseButton() != -1) {
		for (ReceiverIt it = inputReceivers.begin(); it != inputReceivers.end(); it++) {
			if ((*it)->InputEnabled()) {
				(*it)->MousePressed(
					INP->GetLastMouseButton(),
					INP->GetLastMouseX(),
					INP->GetLastMouseY(),
					true
				);
			}
		}
	}
}



void CInputHandler::MouseMoved(SDL_Event* e) {
	for (ReceiverIt it = inputReceivers.begin(); it != inputReceivers.end(); it++) {
		if ((*it)->InputEnabled()) {
			(*it)->MouseMoved(e->motion.x, e->motion.y, e->motion.xrel, e->motion.yrel);
		}
	}
}
void CInputHandler::MousePressed(SDL_Event* e) {
	for (ReceiverIt it = inputReceivers.begin(); it != inputReceivers.end(); it++) {
		if ((*it)->InputEnabled()) {
			(*it)->MousePressed((e->button).button, e->motion.x, e->motion.y, false);
		}
	}
}
void CInputHandler::MouseReleased(SDL_Event* e) {
	for (ReceiverIt it = inputReceivers.begin(); it != inputReceivers.end(); it++) {
		if ((*it)->InputEnabled()) {
			(*it)->MouseReleased((e->button).button, e->motion.x, e->motion.y);
		}
	}
}

void CInputHandler::KeyPressed(SDL_Event* e) {
	// check if key <i> was already pressed earlier
	// note: not really needed with repeat enabled
	// note: does not work for modifiers anyway
	const int i = e->key.keysym.sym;
	const bool r = !!keys[i];

	// assert(keys[i] == 1);
	for (ReceiverIt it = inputReceivers.begin(); it != inputReceivers.end(); it++) {
		if ((*it)->InputEnabled()) {
			(*it)->KeyPressed(e->key.keysym.sym, r);
		}
	}
}
void CInputHandler::KeyReleased(SDL_Event* e) {
	// assert(keys[i] == 0);
	for (ReceiverIt it = inputReceivers.begin(); it != inputReceivers.end(); it++) {
		if ((*it)->InputEnabled()) {
			(*it)->KeyReleased(e->key.keysym.sym);
		}
	}
}

void CInputHandler::WindowExposed(SDL_Event*) {
	for (ReceiverIt it = inputReceivers.begin(); it != inputReceivers.end(); it++) {
		if ((*it)->InputEnabled()) {
			(*it)->WindowExposed();
		}
	}
}
void CInputHandler::WindowResized(SDL_Event* e) {
	switch (e->type) {
		case SDL_VIDEORESIZE: {
			for (ReceiverIt it = inputReceivers.begin(); it != inputReceivers.end(); it++) {
				if ((*it)->InputEnabled()) {
					(*it)->WindowResized(e->resize.w, e->resize.h, -1);
				}
			}
		} break;
		case SDL_ACTIVEEVENT: {
			// SDL_APPACTIVE (gain 0: hidden, gain 1: restored)
			// SDL_APPMOUSEFOCUS
			// SDL_APPINPUTFOCUS
			if (e->active.state & SDL_APPACTIVE) {
				for (ReceiverIt it = inputReceivers.begin(); it != inputReceivers.end(); it++) {
					if ((*it)->InputEnabled()) {
						(*it)->WindowResized(-1, -1, e->active.gain);
					}
				}
			}
		} break;
	}
}



/*
static void KeyPressed(SDL_Event* e) { keys[e->key.keysym.sym] = 1; }
static void KeyReleased(SDL_Event* e) { keys[e->key.keysym.sym] = 0; }
static void MouseMoved(SDL_Event* e) {  e->motion.x, e->motion.y, e->motion.xrel, e->motion.yrel;  }
static void MousePressed(SDL_Event* e) {  mouse[(e->button).button] = 1; e->motion.x; e->motion.y; }
static void MouseReleased(SDL_Event* e) {  mouse[(e->button).button] = 0; e->motion.x; e->motion.y;  }

static void ReadInput(bool* quit) {
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_QUIT: {
				*quit = true;
			} break;

			case SDL_KEYDOWN: { KeyPressed(&event);  } break;
			case SDL_KEYUP:   { KeyReleased(&event); } break;

			case SDL_MOUSEMOTION:     { MouseMoved(&event);    } break;
			case SDL_MOUSEBUTTONDOWN: { MousePressed(&event);  } break;
			case SDL_MOUSEBUTTONUP:   { MouseReleased(&event); } break;
		}
	}
}
static void Loop() {
	bool quit = false;

	while (!quit) {
		ReadInput(&quit);
		ProcessInput(&quit);
	}
}
*/
