#ifndef PFFG_CAMERA_HDR
#define PFFG_CAMERA_HDR

#include <cmath>

#include "../Input/InputReceiver.hpp"
#include "../Math/mat44fwd.hpp"
#include "../Math/mat44.hpp"
#include "../Math/vec3fwd.hpp"
#include "../Math/vec3.hpp"
#include "../Math/Trig.hpp"

enum CAMERA_MOTIONS {
	CAM_YAW     = 0,
	CAM_PITCH   = 1,
	CAM_ROLL    = 2,
	CAM_MOVE    = 3,
	CAM_HSTRAFE = 4,
	CAM_VSTRAFE = 5
};
enum CAMERA_MOVE_MODE {CAM_MOVE_MODE_FPS = 0, CAM_MOVE_MODE_ORBIT = 1, CAM_MOVE_MODE_UNKNOWN = 2};
enum CAMERA_PROJ_MODE {CAM_PROJ_MODE_PERSP = 0, CAM_PROJ_MODE_ORTHO = 1, CAM_PROJ_MODE_UNKNOWN = 2};

struct Camera: public CInputReceiver {
public:
	Camera(const vec3f&, const vec3f&, CAMERA_MOVE_MODE, CAMERA_PROJ_MODE);
	virtual ~Camera();

	bool AABBInOriginPlane(const vec3f& plane, const vec3f& mins, const vec3f& maxs) const;
	bool InView(const vec3f& mins, const vec3f& maxs) const;
	bool InView(const vec3f& pos, float radius = 0.0f) const;
	vec3f GetPixelDir(int x, int y) const;

	void UpdateCoorSys();
	void UpdateFrustum();
	void UpdateHFOV(float);
	void UpdateVFOV(float);
	void Clamp();

	void SetState(const Camera*);

	virtual void Update();

	virtual void RotateX(float, bool) {}
	virtual void RotateY(float, bool) {}
	virtual void RotateZ(float, bool) {}

	virtual void Yaw(    int, float) {}
	virtual void Pitch(  int, float) {}
	virtual void Roll(   int, float) {}
	virtual void Move(   int, float) {}
	virtual void HStrafe(int, float) {}
	virtual void VStrafe(int, float) {}

	const float* GetViewMatrix();
	const float* GetProjMatrix();
	void ApplyViewProjTransform();

	vec3f MouseToWorldCoors(int, int);

	protected: mat44f mat;    // shadow copy of (pos, xdir, ydir, zdir)
	public:    vec3f  vrp;    // point relative to pos determining zdir
	public:    vec3f  pos;    // world-space camera ("eye") location
	public:    vec3f  xdir;   // "right"   dir (vRi) in world-coors
	public:    vec3f  ydir;   // "up"      dir (vUp) in world-coors
	public:    vec3f  zdir;   // "forward" dir (vFo) in world-coors

	vec3f frustumR;    // right view-frustrum plane
	vec3f frustumL;    // left view-frustrum plane
	vec3f frustumB;    // bottom view-frustrum plane
	vec3f frustumT;    // top view-frustrum plane

	float hFOVdeg;     // horizontal FOV (in degrees)
	float vFOVdeg;     // vertical FOV (in degrees)
	float hhFOVrad;    // half of hVOFdeg (in radians)
	float hvFOVrad;    // half of vVOFdeg (in radians)
	float thhFOVrad;   // tangent of hhFOVrad
	float thvFOVrad;   // tangent of hvFOVrad
	float hAspRat;     // aspect ratio (vpsx / vpsy)
	float zNear;
	float zFar;

	CAMERA_MOVE_MODE moveMode;
	CAMERA_PROJ_MODE projMode;

private:
	const float* GetProjMatrixPersp();
	const float* GetProjMatrixOrtho();

	mat44f viewMatrix;
	mat44f projMatrix;
};



struct FPSCamera: public Camera {
	FPSCamera(const vec3f& p, const vec3f& t, CAMERA_PROJ_MODE pm): Camera(p, t, CAM_MOVE_MODE_FPS, pm) {
	}

	void KeyPressed(int, bool);
	void MousePressed(int, int, int, bool);
	void MouseMoved(int, int, int, int);

	void Yaw(int sign, float sens);
	void Pitch(int sign, float sens);
	void Roll(int sign, float sens);

	// forward and backward (along zdir)
	void Move(int sign, float sens);
	// left and right (along xdir)
	void HStrafe(int sign, float sens);
	// up and down (along ydir)
	void VStrafe(int sign, float sens);

	void RotateX(float angle, bool b);
	void RotateY(float angle, bool b);
	void RotateZ(float angle, bool b);
};

struct OrbitCamera: public Camera {
public:
	OrbitCamera(const vec3f& p, const vec3f& t, CAMERA_PROJ_MODE pm): Camera(p, t, CAM_MOVE_MODE_ORBIT, pm) {
		Init(p, t);
	}

	void Init(const vec3f& p, const vec3f& t);

	vec3f GetOrbitPos() const;

	void KeyPressed(int, bool);
	void KeyReleased(int);
	void MousePressed(int, int, int, bool);
	void MouseMoved(int, int, int, int);

	void HStrafe(int sign, float sens);
	void VStrafe(int sign, float sens);

private:
	bool canOrbit;

	float distance, cDistance;      // world-space distance from <pos> to <cen>
	float rotation, cRotation;      // in degrees (hor. rot. is wrt. world x-axis)
	float elevation, cElevation;    // in degrees

	vec3f cen;                      // world-space position that we orbit
};

#endif
