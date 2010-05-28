#include "./CameraController.hpp"
#include "./Camera.hpp"
#include "../System/EngineAux.hpp"
#include "../System/LuaParser.hpp"

CCameraController::CCameraController(): currCam(0x0) {
	const LuaTable* rootTable = LUA->GetRoot();
	const LuaTable* cameraTable = rootTable->GetTblVal("camera");
	const LuaTable* vportTable = rootTable->GetTblVal("viewport");

	const vec3f camPos = cameraTable->GetVec<vec3f>("pos", 3);
	const vec3f camVRP = cameraTable->GetVec<vec3f>("vrp", 3);

	const unsigned int vxsize = unsigned(vportTable->GetFltVal("xsize", 800));
	const unsigned int vysize = unsigned(vportTable->GetFltVal("ysize", 600));

	const float hAspRat = float(vxsize) / float(vysize);

	const float vFOVdeg = cameraTable->GetFltVal("vFOV", 45.0f);
	const float hFOVdeg = RAD2DEG(atanf(hAspRat * tanf(DEG2RAD(vFOVdeg * 0.5f))) * 2.0f);

	const float zNearDist = cameraTable->GetFltVal("zNearDist",     1.0f);
	const float zFarDist  = cameraTable->GetFltVal("zFarDist",  32768.0f);

	const std::string mms = cameraTable->GetStrVal("moveMode", "fps");
	const std::string pms = cameraTable->GetStrVal("projMode", "persp");

	CAMERA_MOVE_MODE moveMode = (mms == "fps"  )? CAM_MOVE_MODE_FPS:    (mms == "orbit")? CAM_MOVE_MODE_ORBIT:  CAM_MOVE_MODE_UNKNOWN;
	CAMERA_PROJ_MODE projMode = (pms == "persp")? CAM_PROJ_MODE_PERSP:  (pms == "ortho")? CAM_PROJ_MODE_ORTHO:  CAM_PROJ_MODE_UNKNOWN;


	// note: integrate this with SwitchCams()?
	switch (moveMode) {
		case CAM_MOVE_MODE_FPS: {
			currCam = new FPSCamera(camPos, camVRP, projMode);
			cameras[CAM_MOVE_MODE_FPS] = currCam;
		} break;
		case CAM_MOVE_MODE_ORBIT: {
			currCam = new OrbitCamera(camPos, camVRP, projMode);
			cameras[CAM_MOVE_MODE_ORBIT] = currCam;
		} break;
		default: {
			assert(false);
		} break;
	}


	currCam->zNear     = zNearDist;
	currCam->zFar      = zFarDist;
	currCam->hAspRat   = hAspRat;

	currCam->vFOVdeg   = vFOVdeg;
	currCam->hvFOVrad  = vFOVdeg * (M_PI / 360.0f);
	currCam->thvFOVrad = tanf(currCam->hvFOVrad);

	currCam->hFOVdeg   = hFOVdeg;
	currCam->hhFOVrad  = hFOVdeg * (M_PI / 360.0f);
	currCam->thhFOVrad = tanf(currCam->hhFOVrad);
}

CCameraController::~CCameraController() {
	for (std::map<int, Camera*>::iterator it = cameras.begin(); it != cameras.end(); it++) {
		delete (it->second);
	}

	currCam = 0x0;
}



void CCameraController::Update() {
	currCam->Update();
}

void CCameraController::SwitchCams() {
	switch (currCam->moveMode) {
		case CAM_MOVE_MODE_FPS: {
			OrbitCamera* nextCam = 0x0;
			std::map<int, Camera*>::iterator it = cameras.find(CAM_MOVE_MODE_ORBIT);

			if (it == cameras.end()) {
				nextCam = new OrbitCamera(currCam->pos, currCam->vrp, CAM_PROJ_MODE_PERSP);
			} else {
				nextCam = dynamic_cast<OrbitCamera*>(it->second);
			}

			// disable the old camera so it
			// does not react to input until
			// we switch back to it
			cameras[CAM_MOVE_MODE_ORBIT] = nextCam;
			nextCam->SetState(currCam);
			nextCam->Init(currCam->pos, currCam->pos + currCam->zdir * 512.0f);
			nextCam->EnableInput();
			currCam->DisableInput();
			currCam = nextCam;
		} break;

		case CAM_MOVE_MODE_ORBIT: {
			FPSCamera* nextCam = 0x0;
			std::map<int, Camera*>::iterator it = cameras.find(CAM_MOVE_MODE_FPS);

			if (it == cameras.end()) {
				nextCam = new FPSCamera(currCam->pos, currCam->vrp, CAM_PROJ_MODE_PERSP);
			} else {
				nextCam = dynamic_cast<FPSCamera*>(it->second);
			}

			cameras[CAM_MOVE_MODE_FPS] = nextCam;
			nextCam->SetState(currCam);
			nextCam->EnableInput();
			currCam->DisableInput();
			currCam = nextCam;
		} break;

		default: {
			assert(false);
		} break;
	}
}