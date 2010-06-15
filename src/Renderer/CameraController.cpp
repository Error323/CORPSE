#include "./CameraController.hpp"
#include "./Camera.hpp"
#include "../System/EngineAux.hpp"
#include "../System/LuaParser.hpp"

CCameraController::CCameraController(): currCam(NULL) {
	const LuaTable* rootTable = LUA->GetRoot();
	const LuaTable* cameraTable = rootTable->GetTblVal("camera");

	const vec3f& camPos = cameraTable->GetVec<vec3f>("pos", 3);
	const vec3f& camVRP = cameraTable->GetVec<vec3f>("vrp", 3);

	const int moveMode = cameraTable->GetFltVal("moveMode", Camera::CAM_MOVE_MODE_FPS);
	const int projMode = cameraTable->GetFltVal("projMode", Camera::CAM_PROJ_MODE_PERSP);

	// note: integrate this with SwitchCams()?
	switch (moveMode) {
		case Camera::CAM_MOVE_MODE_FPS: {
			currCam = new FPSCamera(camPos, camVRP, projMode);
			currCam->Init(currCam->pos, currCam->pos + currCam->zdir);
			cameras[Camera::CAM_MOVE_MODE_FPS] = currCam;
		} break;

		case Camera::CAM_MOVE_MODE_ORBIT: {
			currCam = new OrbitCamera(camPos, camVRP, projMode);
			currCam->Init(currCam->pos, currCam->pos + currCam->zdir);
			cameras[Camera::CAM_MOVE_MODE_ORBIT] = currCam;
		} break;

		case Camera::CAM_MOVE_MODE_OVERHEAD: {
			currCam = new OverheadCamera(camPos, camVRP, projMode);
			currCam->Init(currCam->pos, currCam->pos + currCam->zdir);
			cameras[Camera::CAM_MOVE_MODE_OVERHEAD] = currCam;
		} break;

		default: {
			assert(false);
		} break;
	}
}

CCameraController::~CCameraController() {
	for (std::map<int, Camera*>::iterator it = cameras.begin(); it != cameras.end(); it++) {
		delete (it->second);
	}

	currCam = NULL;
}



void CCameraController::Update() {
	currCam->Update();
}

void CCameraController::SwitchCams() {
	Camera* nextCam = NULL;

	switch (currCam->moveMode) {
		case Camera::CAM_MOVE_MODE_FPS: {
			// first-person switches to orbit
			std::map<int, Camera*>::iterator it = cameras.find(Camera::CAM_MOVE_MODE_ORBIT);

			if (it == cameras.end()) {
				nextCam = new OrbitCamera(currCam->pos, currCam->vrp, currCam->projMode);
			} else {
				nextCam = dynamic_cast<OrbitCamera*>(it->second);
			}

			cameras[Camera::CAM_MOVE_MODE_ORBIT] = nextCam;
		} break;

		case Camera::CAM_MOVE_MODE_ORBIT: {
			// orbit switches to overhead
			std::map<int, Camera*>::iterator it = cameras.find(Camera::CAM_MOVE_MODE_OVERHEAD);

			if (it == cameras.end()) {
				nextCam = new OverheadCamera(currCam->pos, currCam->vrp, currCam->projMode);
			} else {
				nextCam = dynamic_cast<OverheadCamera*>(it->second);
			}

			cameras[Camera::CAM_MOVE_MODE_OVERHEAD] = nextCam;
		} break;

		case Camera::CAM_MOVE_MODE_OVERHEAD: {
			// overhead switches to first-person
			std::map<int, Camera*>::iterator it = cameras.find(Camera::CAM_MOVE_MODE_FPS);

			if (it == cameras.end()) {
				nextCam = new FPSCamera(currCam->pos, currCam->vrp, currCam->projMode);
			} else {
				nextCam = dynamic_cast<FPSCamera*>(it->second);
			}

			cameras[Camera::CAM_MOVE_MODE_FPS] = nextCam;
		} break;

		default: {
			assert(false);
		} break;
	}

	// disable the old camera so it does not react to input until
	// we switch back to it; also set the internal parameters of
	// the next camera since the viewport might have been resized
	// while it was inactive
	nextCam->SetState(currCam);
	nextCam->SetInternalParameters();
	nextCam->Init(currCam->pos, currCam->pos + currCam->zdir);
	nextCam->EnableInput();
	currCam->DisableInput();
	currCam = nextCam;
}
