#ifndef PFFG_SCENE_HDR
#define PFFG_SCENE_HDR

#include <vector>

#include "../System/IEvent.hpp"
#include "../System/IEventReceiver.hpp"
#include "../Math/vec4fwd.hpp"

class CSMFRenderer;
class CSkyBox;
struct Camera;
struct LocalModel;

class CScene: public IEventReceiver {
public:
	CScene();
	~CScene();

	void Draw(Camera*);

	bool WantsEvent(int) const;
	void OnEvent(const IEvent*);

private:
	void LoadTeamColors();
	void LoadObjectModel(unsigned int);
	void LoadObjectModels();
	void DrawModels(Camera*, bool);
	void DrawMapAndModels(Camera*, bool);
	void SetBoundingRadius();
	void InitLight();

	Camera* sun;
	CSMFRenderer* smfRenderer;
	CSkyBox* skyBox;

	// radius in world-coordinates of the
	// sphere minimally encompassing the
	// map
	float boundingRadiusSq;
	float boundingRadius;

	// note: store color in object or model instead?
	std::vector<vec4f> teamColors;

	// NOTE: these should be members of RenderThread
	unsigned int simFrameIntervalTicks;
	float        simFrameDeltaTickRatio;

	unsigned int currSimFrame;
	unsigned int currSimFrameTick;
};

#endif
