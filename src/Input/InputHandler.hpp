#ifndef PFFG_INPUTHANDLER_HDR
#define PFFG_INPUTHANDLER_HDR

#include <set>
#include <vector>

#include "../Math/vec3fwd.hpp"
#include "../Math/vec3.hpp"

union SDL_Event;
class CInputReceiver;

class CInputHandler {
public:
	static CInputHandler* GetInstance();
	static void FreeInstance(CInputHandler*);

	void AddReceiver(CInputReceiver*);
	void DelReceiver(CInputReceiver*);
	void Update();

	float GetKeySensitivity() const { return keySens; }
	float GetMouseSensitivity() const { return mouseSens; }

	int GetLastMouseButton() const { return lastMouseButton; }
	const vec3i& GetCurrMouseCoors() const { return currMouseCoors; }
	const vec3i& GetLastMouseCoors() const { return lastMouseCoors; }

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


	vec3i currMouseCoors;
	vec3i lastMouseCoors;

	int lastMouseButton;
	unsigned int lastInputTick;
	unsigned int inputFrameRate;
	unsigned int inputFrameTime;
	float keySens;
	float mouseSens;
};

#define inputHandler (CInputHandler::GetInstance())

#endif
