#ifndef PFFG_WINDOW_HDR
#define PFFG_WINDOW_HDR

#include <list>

// TODO: figure out connection to EngineAux::WindowState
class ViewPort;

class IWindow {
	public:
		virtual void Update() = 0;
		virtual void SetSize(int, int) = 0;
		virtual void MovePosition(int, int) = 0;
		virtual void LoadPosition(int, int) = 0;
		virtual void SavePosition(int, int) = 0;

		struct State {
			int x, y;        // (x, y) of top-left? corner
			int w, h;        // (w, h) dimensions
			bool fullScreen; // not the same as maximized
			bool minimized;
		};

		virtual const State& GetState() {
			return state;
		}
		virtual const std::list<ViewPort*>& GetPorts() {
			return ports;
		}

	protected:
		State state;
		std::list<ViewPort*> ports;
};

#endif
