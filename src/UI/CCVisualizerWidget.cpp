#include <SDL/SDL.h>
#include <GL/gl.h>
#include <FTGL/ftgl.h>

#include "./UI.hpp"
#include "./CCWidget.hpp"
#include "../Math/vec3.hpp"
#include "../Renderer/RenderThread.hpp"
#include "../Sim/SimThread.hpp"
#include "../System/EngineAux.hpp"
#include "../Path/IPathModule.hpp"

void ui::CCWidget::Update(const vec3i&, const vec3i& size) {
	const IPathModule* m = simThread->GetPathModule();
}
