#ifndef PFFG_SCENE_HDR
#define PFFG_SCENE_HDR

#include <list>

#include "../System/IEvent.hpp"
#include "../System/IEventReceiver.hpp"

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
};

#endif
