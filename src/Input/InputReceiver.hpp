#ifndef PFFG_INPUTRECEIVER_HDR
#define PFFG_INPUTRECEIVER_HDR

class CInputReceiver {
public:
	CInputReceiver(): inputEnabled(true) {
	}

	virtual void MouseMoved(int, int, int, int) {}
	virtual void MousePressed(int, int, int, bool) {}
	virtual void MouseReleased(int, int, int) {}
	virtual void KeyPressed(int, bool) {}
	virtual void KeyReleased(int) {}

	virtual void WindowExposed() {}
	virtual void WindowResized(int, int) {}

	virtual bool InputEnabled() const { return inputEnabled; }
	virtual void EnableInput() { inputEnabled = true; }
	virtual void DisableInput() { inputEnabled = false; }

protected:
	bool inputEnabled;
};

#endif
