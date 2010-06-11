#include <SDL/SDL.h>

#include "../System/EngineAux.hpp"
#include "../System/LuaParser.hpp"
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
	buts.resize(SDLK_LAST, 0); // LEFT, MIDDLE, RIGHT, WHEELDOWN, WHEELUP, ?

	// X----------X--X--X--X--X--X--X
	// SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY >> 2, SDL_DEFAULT_REPEAT_INTERVAL);
	SDL_EnableUNICODE(1);
	SDL_SetModState(KMOD_NONE);
	SDL_ShowCursor(AUX->GetMouseLook()? SDL_DISABLE: SDL_ENABLE);
	SDL_WM_GrabInput(AUX->GetMouseLook()? SDL_GRAB_ON: SDL_GRAB_OFF);


	const LuaTable* rootTable = LUA->GetRoot();
	const LuaTable* inputTable = rootTable->GetTblVal("input");

	inputFrameRate = inputTable->GetFltVal("inputRate", 100);
	inputFrameTime = 1000 / inputFrameRate;

	keySens   = inputFrameTime * inputTable->GetFltVal("keySens", 50.0f) * 0.01f;
	mouseSens = inputFrameTime * inputTable->GetFltVal("mouseSens", 20.0f) * 0.01f;

	currMouseCoors.x = -1; lastMouseCoors.x = -1;
	currMouseCoors.y = -1; lastMouseCoors.y = -1;

	lastMouseButton = -1;
	lastInputTick = 0;
}

CInputHandler::~CInputHandler() {
	SDL_WM_GrabInput(SDL_GRAB_OFF);

	keys.clear();
	buts.clear();
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
	if ((SDL_GetTicks() - lastInputTick) >= inputFrameTime) {
		lastInputTick = SDL_GetTicks();
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
				WindowExposed(&event);
			} break;


			case SDL_QUIT: {
				// closing via the WM
				AUX->SetWantQuit(true);
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

	// process the input to generate extra
	// KeyPressed and MousePressed events
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

	// SDL's own key-repeat functionality is not
	// fast enough, so we take a snapshot of the
	// keyboard state every frame and fire events
	// ourselves
	// note: <numKeys> is always equal to SDLK_LAST
	// (keys.size()) no matter how many are actually
	// pressed
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
	Uint8 state = SDL_GetMouseState(&currMouseCoors.x, &currMouseCoors.y);

	if (state == 0) {
		lastMouseButton = -1;
	} else {
		if ((state & SDL_BUTTON_LMASK) || (state & SDL_BUTTON_MMASK) || (state & SDL_BUTTON_RMASK)) {
			for (ReceiverIt it = inputReceivers.begin(); it != inputReceivers.end(); it++) {
				if ((*it)->InputEnabled()) {
					(*it)->MousePressed(lastMouseButton, lastMouseCoors.x, lastMouseCoors.y, true);
				}
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

	if (AUX->GetMouseLook()) {
		// re-center the mouse and eat the event it generates
		// note: this can also eat MouseReleased() events and
		// cause auto-move unless WE update the mouse state
		SDL_WarpMouse(WIN->GetWindowSize().x >> 1, WIN->GetWindowSize().y >> 1);
		SDL_Event e;
		while (SDL_PollEvent(&e)) {}
	}
}
void CInputHandler::MousePressed(SDL_Event* e) {
	const int button = (e->button).button;
	const bool repeat = !!buts[button];

	buts[button] = 1;

	for (ReceiverIt it = inputReceivers.begin(); it != inputReceivers.end(); it++) {
		if ((*it)->InputEnabled()) {
			(*it)->MousePressed(button, e->motion.x, e->motion.y, repeat);
		}
	}

	if (!repeat) {
		lastMouseButton = button;
		lastMouseCoors.x = e->motion.x;
		lastMouseCoors.y = e->motion.y;
	}
}
void CInputHandler::MouseReleased(SDL_Event* e) {
	buts[(e->button).button] = 0;

	for (ReceiverIt it = inputReceivers.begin(); it != inputReceivers.end(); it++) {
		if ((*it)->InputEnabled()) {
			(*it)->MouseReleased((e->button).button, e->motion.x, e->motion.y);
		}
	}

	lastMouseButton = -1;
	lastMouseCoors.x = e->motion.x;
	lastMouseCoors.y = e->motion.y;
}

void CInputHandler::KeyPressed(SDL_Event* e) {
	// check if key <i> was already pressed earlier
	// note:
	//   this is not really needed with repeat enabled,
	//   and does not work at all for modifiers anyway
	const int key = e->key.keysym.sym;
	const bool repeat = !!keys[key];

	keys[key] = 1;

	for (ReceiverIt it = inputReceivers.begin(); it != inputReceivers.end(); it++) {
		if ((*it)->InputEnabled()) {
			(*it)->KeyPressed(key, repeat);
		}
	}
}
void CInputHandler::KeyReleased(SDL_Event* e) {
	keys[e->key.keysym.sym] = 0;

	for (ReceiverIt it = inputReceivers.begin(); it != inputReceivers.end(); it++) {
		if ((*it)->InputEnabled()) {
			(*it)->KeyReleased(e->key.keysym.sym);
		}
	}
}

void CInputHandler::WindowExposed(SDL_Event* e) {
	if (e->type == SDL_ACTIVEEVENT) {
		// possible masks are
		//     SDL_APPACTIVE (gain 0 means hidden, gain 1 means restored)
		//     SDL_APPMOUSEFOCUS
		//     SDL_APPINPUTFOCUS
		if (e->active.state & SDL_APPACTIVE) {
			AUX->SetWantDraw(e->active.gain);
		}
	}

	for (ReceiverIt it = inputReceivers.begin(); it != inputReceivers.end(); it++) {
		if ((*it)->InputEnabled()) {
			(*it)->WindowExposed();
		}
	}
}
void CInputHandler::WindowResized(SDL_Event* e) {
	assert(e->type == SDL_VIDEORESIZE);

	for (ReceiverIt it = inputReceivers.begin(); it != inputReceivers.end(); it++) {
		if ((*it)->InputEnabled()) {
			(*it)->WindowResized(e->resize.w, e->resize.h);
		}
	}
}
