#ifndef PFFG_CAMERA_CONTROLLER_HDR
#define PFFG_CAMERA_CONTROLLER_HDR

#include <map>

struct Camera;
class CCameraController {
public:
	CCameraController();
	~CCameraController();

	void Update();
	void SwitchCams();

	Camera* GetCurrCam() { return currCam; }

private:
	std::map<int, Camera*> cameras;
	Camera* currCam;
};

#endif

