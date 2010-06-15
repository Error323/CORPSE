#ifndef PFFG_CAMERA_HDR
#define PFFG_CAMERA_HDR

#include <cmath>

#include "../Input/InputReceiver.hpp"
#include "../Math/mat44fwd.hpp"
#include "../Math/mat44.hpp"
#include "../Math/vec3fwd.hpp"
#include "../Math/vec3.hpp"
#include "../Math/Trig.hpp"

struct Camera: public CInputReceiver {
public:
	friend class CCameraController;

	enum {
		CAM_YAW     = 0,
		CAM_PITCH   = 1,
		CAM_ROLL    = 2,
		CAM_MOVE    = 3,
		CAM_HSTRAFE = 4,
		CAM_VSTRAFE = 5
	};

	enum {
		CAM_MOVE_MODE_FPS      = 0, 
		CAM_MOVE_MODE_ORBIT    = 1, 
		CAM_MOVE_MODE_OVERHEAD = 2, 
		CAM_MOVE_MODE_LAST     = 3
	};

	enum {
		CAM_PROJ_MODE_PERSP = 0, 
		CAM_PROJ_MODE_ORTHO = 1,
		CAM_PROJ_MODE_LAST  = 2
	};

	Camera(const vec3f&, const vec3f&, int, int);
	virtual ~Camera();

	bool AABBInOriginPlane(const vec3f& plane, const vec3f& mins, const vec3f& maxs) const;
	bool InView(const vec3f& mins, const vec3f& maxs) const;
	bool InView(const vec3f& pos, float radius = 0.0f) const;

	vec3f GetPixelDir(int x, int y) const;
	vec3f ScreenToWorldCoors(int, int);

	void WindowResized(int, int);

	virtual void Init(const vec3f&, const vec3f&) {}
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

	virtual bool Active() const { return false; }

	const float* GetViewMatrix();
	const float* GetProjMatrix();
	void ApplyViewProjTransform();

	protected: mat44f mat;         // shadow copy of (pos, xdir, ydir, zdir)
	public:    vec3f  vrp;         // point relative to pos determining zdir
	public:    vec3f  pos;         // world-space camera ("eye") location
	public:    vec3f  xdir;        // "right"   dir (vRi) in world-coors
	public:    vec3f  ydir;        // "up"      dir (vUp) in world-coors
	public:    vec3f  zdir;        // "forward" dir (vFo) in world-coors

	vec3f frustumL, frustumR;      // {left, right} view-frustrum plane
	vec3f frustumB, frustumT;      // {bottom, top} view-frustrum plane

	float    vFOVdeg,    hFOVdeg;  // {horizontal, vertical} FOV angle (in degrees)
	float    hFOVrad,    vFOVrad;  // {horizontal, vertical} FOV angle (in radians)
	float   hhFOVrad,   hvFOVrad;  // half of PhVOFrad, vVOFrad} angle (in radians)
	float  thhFOVrad,  thvFOVrad;  // tangent of {hhFOVrad, hvFOVrad} (side ratio, not "in radians")
	float ithhFOVrad, ithvFOVrad;  // reciprocal of {thhFOVrad, thvFOVrad}

	float hAspectRatio;            // horizontal viewport aspect ratio (W / H)
	float vAspectRatio;            // vertical viewport aspect ratio (H / W)

	float zNearDistance;
	float zFarDistance;

	int moveMode;
	int projMode;

protected:
	void SetState(const Camera*);
	void SetInternalParameters();
	void UpdateCoorSys();
	void UpdateFrustum();
	void UpdateHFOV(float);
	void UpdateVFOV(float);

	const float* GetProjMatrixPersp();
	const float* GetProjMatrixOrtho();

	mat44f viewMatrix;
	mat44f projMatrix;
};


struct FPSCamera: public Camera {
	FPSCamera(const vec3f& p, const vec3f& t, int projectionMode): Camera(p, t, CAM_MOVE_MODE_FPS, projectionMode) {
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

	bool Active() const;
};

struct OrbitCamera: public Camera {
public:
	OrbitCamera(const vec3f& p, const vec3f& t, int projectionMode): Camera(p, t, CAM_MOVE_MODE_ORBIT, projectionMode) {
	}

	void Init(const vec3f& p, const vec3f& t);

	vec3f GetOrbitPos() const;

	void KeyPressed(int, bool);
	void KeyReleased(int);
	void MousePressed(int, int, int, bool);
	void MouseMoved(int, int, int, int);

	void HStrafe(int sign, float sens);
	void VStrafe(int sign, float sens);

	bool Active() const { return active; }

private:
	bool active;

	float distance, cDistance;      // world-space distance from <pos> to <cen>
	float rotation, cRotation;      // in degrees (hor. rot. is wrt. world x-axis)
	float elevation, cElevation;    // in degrees

	vec3f cen;                      // world-space position that we orbit
};


// A standard RTS overhead target-camera
struct OverheadCamera: public Camera {
	OverheadCamera(const vec3f& p, const vec3f& t, int projectionMode): Camera(p, t, CAM_MOVE_MODE_OVERHEAD, projectionMode) {
	}

	void Init(const vec3f& p, const vec3f& t);

	void KeyPressed(int, bool);
	void KeyReleased(int);
	void MousePressed(int, int, int, bool);
	void MouseMoved(int, int, int, int);

	void ScrollNorthSouth(int sign, float sens);
	void ScrollEastWest(int sign, float sens);
	void Zoom(int sign, float sens);

	float sensMultiplier;
	vec3f target;

	// bool Active() const;
};

#endif
