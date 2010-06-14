#ifndef PFFG_UI_HDR
#define PFFG_UI_HDR

#include "../Input/InputReceiver.hpp"

class IFontManager;
class FTFont;

struct SimObjectSelector;
struct SimObjectSpawner;

class UI: public CInputReceiver {
public:
	static UI* GetInstance();
	static void FreeInstance(UI*);

	void Update();

	void MouseMoved(int, int, int, int);
	void MousePressed(int, int, int, bool);
	void MouseReleased(int, int, int);

private:
	UI();
	~UI();

	IFontManager* mFontManager;
	FTFont* mFont;

	SimObjectSelector* mSimObjectSelector;
	SimObjectSpawner* mSimObjectSpawner;
};

#endif
