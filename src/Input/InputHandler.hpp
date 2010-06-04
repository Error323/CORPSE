#ifndef PFFG_INPUTHANDLER_HDR
#define PFFG_INPUTHANDLER_HDR

#include <set>
#include <vector>

union SDL_Event;
class CInputReceiver;

class CInputHandler {
public:
	static CInputHandler* GetInstance();
	static void FreeInstance(CInputHandler*);

	void AddReceiver(CInputReceiver*);
	void DelReceiver(CInputReceiver*);
	void Update();

private:
	CInputHandler();
	~CInputHandler();

	void UpdateKeyState();
	void UpdateMouseState();

	void MouseMoved(SDL_Event*);
	void MousePressed(SDL_Event*);
	void MouseReleased(SDL_Event*);
	void KeyPressed(SDL_Event*);
	void KeyReleased(SDL_Event*);

	void WindowExposed(SDL_Event*);
	void WindowResized(SDL_Event*);

	std::vector<unsigned char> keys;
	std::vector<unsigned char> buts;

	std::set<CInputReceiver*> inputReceivers;
	typedef std::set<CInputReceiver*>::iterator ReceiverIt;
};

#define inputHandler (CInputHandler::GetInstance())

#endif
