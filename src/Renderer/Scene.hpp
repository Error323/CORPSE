#ifndef PFFG_SCENE_HDR
#define PFFG_SCENE_HDR

#include <list>

class CSMFRenderer;
class CSkyBox;
struct Camera;
struct LocalModel;

class CScene {
public:
	CScene();
	~CScene();

	void Draw(Camera*);

private:
	void DrawModels(Camera*, bool);
	void DrawMapAndModels(Camera*, bool);
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
