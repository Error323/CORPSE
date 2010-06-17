#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL/SDL.h>

#include "./Camera.hpp"
#include "../UI/Window.hpp"
#include "../Input/InputHandler.hpp"
#include "../System/EngineAux.hpp"
#include "../System/LuaParser.hpp"

Camera::Camera(const vec3f& p, const vec3f& t, int movementMode, int projectionMode):
	mat(p, XVECf, YVECf, ZVECf),
	vrp(t),
	pos(p),
	xdir(mat.GetXDir()),
	ydir(mat.GetYDir()),
	zdir(mat.GetZDir()),
	moveMode(movementMode),
	projMode(projectionMode) {

	inputHandler->AddReceiver(this);

	SetInternalParameters();
	Update();
}
Camera::~Camera() {
	inputHandler->DelReceiver(this);
}

bool Camera::AABBInOriginPlane(const vec3f& plane, const vec3f& mins, const vec3f& maxs) const {
	vec3f fp;
		fp.x = (plane.x > 0.0f)? mins.x: maxs.x;
		fp.y = (plane.y > 0.0f)? mins.y: maxs.y;
		fp.z = (plane.z > 0.0f)? mins.z: maxs.z;
	return (plane.dot3D(fp - pos) < 0.0f);
}

// returns true iif all corners of bounding-box defined
// by <mins, maxs> lie on the "positive" side of each of
// the frustum's half-spaces (?)
//
// if using quad-tree visibility tests, we can not say
// "if no corner in view, then entire quad invisible"
// (area in between corners might be within frustum)
// proper way would be:
//     all points left of left-plane ==> invisible
//     all points right of right-plane ==> invisible
//     all points above top-plane ==> invisible
//     all points below bottom-plane ==> invisible
//     all points before near-plane ==> invisible
//     all points behind far-plane ==> invisible
bool Camera::InView(const vec3f& mins, const vec3f& maxs) const {
	return
		AABBInOriginPlane(frustumR, mins, maxs) &&
		AABBInOriginPlane(frustumL, mins, maxs) &&
		AABBInOriginPlane(frustumB, mins, maxs) &&
		AABBInOriginPlane(frustumT, mins, maxs);
}
bool Camera::InView(const vec3f& p, float radius) const {
	const vec3f t = (p - pos);

	return
		(t.dot3D(frustumR) < radius) &&
		(t.dot3D(frustumL) < radius) &&
		(t.dot3D(frustumB) < radius) &&
		(t.dot3D(frustumT) < radius);
}

vec3f Camera::GetPixelDir(int x, int y) const {
	const int vppx = gWindow->GetViewPort().pos.x;
	const int vpsx = gWindow->GetViewPort().size.x;
	const int vpsy = gWindow->GetViewPort().size.y;

	const float dx = ((x - vppx - vpsx * 0.5f) / vpsy) * (thvFOVrad * 2.0f);
	const float dy = ((y - vpsy        * 0.5f) / vpsy) * (thvFOVrad * 2.0f);

	return ((zdir - (ydir * dy) + (xdir * dx)).norm());
}

void Camera::SetInternalParameters() {
	const LuaTable* rootTable = LUA->GetRoot();
	const LuaTable* cameraTable = rootTable->GetTblVal("camera");

	hAspectRatio = float(gWindow->GetViewPort().size.x) / float(gWindow->GetViewPort().size.y);
	vAspectRatio = float(gWindow->GetViewPort().size.y) / float(gWindow->GetViewPort().size.x);

	vFOVdeg = cameraTable->GetFltVal("vFOV", 45.0f);
	vFOVrad = DEG2RAD(vFOVdeg);
	// hFOVdeg = RAD2DEG(atanf(hAspectRatio * tanf(DEG2RAD(vFOVdeg * 0.5f))) * 2.0f);
	// hFOVrad = DEG2RAD(hFOVdeg);
	hFOVrad = (atanf(hAspectRatio * tanf(vFOVrad * 0.5f)) * 2.0f);
	hFOVdeg = RAD2DEG(hFOVrad);

	hhFOVrad = hFOVrad * 0.5f; thhFOVrad = tanf(hhFOVrad); ithhFOVrad = 1.0f / thhFOVrad;
	hvFOVrad = vFOVrad * 0.5f; thvFOVrad = tanf(hvFOVrad); ithvFOVrad = 1.0f / thvFOVrad;

	// vFOV and aspect-ratio together do not fully determine
	// the projection; also need the z-distance parameters
	zNearDistance = cameraTable->GetFltVal("zNearDist",     1.0f);
	zFarDistance  = cameraTable->GetFltVal("zFarDist",  32768.0f);

	// viewPlane.x = camVP.x;
	// viewPlane.y = camVP.y;
	// viewPlane.z = (camVP.y * 0.5f) / tanf(DEG2RAD(vFOVdeg * 0.5f));
}

void Camera::WindowResized(int, int) {
	// recalculate the aspect- ratios
	SetInternalParameters();
}



void Camera::UpdateCoorSys() {
	zdir = (vrp - pos).inorm();
	xdir = (zdir.cross(ydir)).inorm();
	ydir = (xdir.cross(zdir)).inorm();
	vrp  = pos + zdir;

	mat.SetPos(pos); //!
	mat.SetXDir(xdir);
	mat.SetYDir(ydir);
	mat.SetZDir(zdir);
}

void Camera::UpdateFrustum() {
	const vec3f zdirY = (-zdir * (    thvFOVrad               ));
	const vec3f zdirX = (-zdir * (tanf(hvFOVrad * hAspectRatio)));

	frustumT = (zdirY + ydir).inorm();
	frustumB = (zdirY - ydir).inorm();
	frustumR = (zdirX + xdir).inorm();
	frustumL = (zdirX - xdir).inorm();
}

void Camera::SetState(const Camera* c) {
	pos  = c->pos;
	xdir = c->xdir;
	ydir = c->ydir;
	zdir = c->zdir;
	vrp  = c->vrp;
	mat  = c->mat;

	hFOVdeg   = c->hFOVdeg;
	hhFOVrad  = c->hhFOVrad;
	thhFOVrad = c->thhFOVrad;
	vFOVdeg   = c->vFOVdeg;
	hvFOVrad  = c->hvFOVrad;
	thvFOVrad = c->thvFOVrad;

	hAspectRatio  = c->hAspectRatio;
	zNearDistance = c->zNearDistance;
	zFarDistance  = c->zFarDistance;
}

void Camera::Update() {
	UpdateCoorSys();
	UpdateFrustum();
}



// same as gluLookAt(cam->posXYZ, cam->vrpXYZ, cam->upXYZ)
// except the matrix needs to be multiplied manually
//
// OpenGL assumes a static camera at (0, 0, 0) looking
// down negative-z, so to provide the illusion that the
// camera is moving through the world we must transform
// everything in it the opposite way. This means that we
// need to transpose the rotation (mat.*dir) _and_ invert
// (by negating) the camera's world-space translation (ie.
// mat.pos) to build the proper OGL view matrix.
//
const float* Camera::GetViewMatrix() {
	const vec3f& p = mat.GetPos();
	const vec3f& z = mat.GetZDir(); // vrp - pos (forward)
	const vec3f& x = mat.GetXDir(); // z cross y (side)
	const vec3f& y = mat.GetYDir(); // x cross z (up)

	// transpose the rotation
	viewMatrix.m[ 0] =  x.x;
	viewMatrix.m[ 4] =  x.y;
	viewMatrix.m[ 8] =  x.z;

	viewMatrix.m[ 1] =  y.x;
	viewMatrix.m[ 5] =  y.y;
	viewMatrix.m[ 9] =  y.z;

	viewMatrix.m[ 2] = -z.x;
	viewMatrix.m[ 6] = -z.y;
	viewMatrix.m[10] = -z.z;

	viewMatrix.m[ 3] =  0.0f;
	viewMatrix.m[ 7] =  0.0f;
	viewMatrix.m[11] =  0.0f;

	// invert the translation and save a
	// glTranslate(-p.x, -p.y, -p.z) call
	viewMatrix.m[12] = x.dot3D(-p);
	viewMatrix.m[13] = y.dot3D(-p);
	viewMatrix.m[14] = z.dot3D( p);
	viewMatrix.m[15] = 1.0f;

	viewMatrix.UpdatePXYZ();
	return (&viewMatrix.m[0]);
}

const float* Camera::GetProjMatrix() {
	switch (projMode) {
		case CAM_PROJ_MODE_PERSP: { return (GetProjMatrixPersp()); } break;
		case CAM_PROJ_MODE_ORTHO: { return (GetProjMatrixOrtho()); } break;
		default: {} break;
	}

	return NULL;
}

// same as gluPerspective(vFOV, aspRat, zNear, zFar)
// except the matrix needs to be multiplied manually
const float* Camera::GetProjMatrixPersp() {
	const float t = zNearDistance * thvFOVrad;
	const float b = -t;
	const float l = b * hAspectRatio;
	const float r = t * hAspectRatio;

	projMatrix.m[ 0] = (2.0f * zNearDistance) / (r - l);
	projMatrix.m[ 1] =  0.0f;
	projMatrix.m[ 2] =  0.0f;
	projMatrix.m[ 3] =  0.0f;

	projMatrix.m[ 4] =  0.0f;
	projMatrix.m[ 5] = (2.0f * zNearDistance) / (t - b);
	projMatrix.m[ 6] =  0.0f;
	projMatrix.m[ 7] =  0.0f;

	projMatrix.m[ 8] = (r + l) / (r - l);
	projMatrix.m[ 9] = (t + b) / (t - b);
	projMatrix.m[10] = -(zFarDistance + zNearDistance) / (zFarDistance - zNearDistance);
	projMatrix.m[11] = -1.0f;

	projMatrix.m[12] =   0.0f;
	projMatrix.m[13] =   0.0f;
	projMatrix.m[14] = -(2.0f * zFarDistance * zNearDistance) / (zFarDistance - zNearDistance);
	projMatrix.m[15] =   0.0f;

	projMatrix.UpdatePXYZ();
	return (&projMatrix.m[0]);
}

// same as
//    glOrtho(-1, 1, -1, 1, znear, zfar)
//    glScalef(wx, wy, 1)
// in orthographic mode, a triangle's projected size
// is invariant wrt. its z-distance, so there exists
// a 1-to-1 correspondence between fragment size and
// world units (in which vertices are specified)
// note: eliminate scaling if we leave VM at identity?
const float* Camera::GetProjMatrixOrtho() {
	const float t =  1.0f * gWindow->GetViewPort().size.y;
	const float b = -1.0f * gWindow->GetViewPort().size.y;
	const float l = -1.0f * gWindow->GetViewPort().size.x;
	const float r =  1.0f * gWindow->GetViewPort().size.x;

	const float tx = -((r + l) / (r - l));
	const float ty = -((t + b) / (t - b));
	const float tz = -((zFarDistance + zNearDistance) / (zFarDistance - zNearDistance));

	projMatrix.m[ 0] =  2.0f / (r - l);
	projMatrix.m[ 1] =  0.0f;
	projMatrix.m[ 2] =  0.0f;
	projMatrix.m[ 3] =  0.0f;

	projMatrix.m[ 4] =  0.0f;
	projMatrix.m[ 5] =  2.0f / (t - b);
	projMatrix.m[ 6] =  0.0f;
	projMatrix.m[ 7] =  0.0f;

	projMatrix.m[ 8] =  0.0f;
	projMatrix.m[ 9] =  0.0f;
	projMatrix.m[10] = -2.0f / (zFarDistance - zNearDistance);
	projMatrix.m[11] =  0.0f;

	projMatrix.m[12] = tx;
	projMatrix.m[13] = ty;
	projMatrix.m[14] = tz;
	projMatrix.m[15] = 1.0f;

	projMatrix.UpdatePXYZ();
	return (&projMatrix.m[0]);
}

void Camera::ApplyViewProjTransform() {
	glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMultMatrixf(GetProjMatrix()); // or glLoadMatrixf()

	glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMultMatrixf(GetViewMatrix()); // or glLoadMatrixf()
}



vec3f Camera::ScreenToWorldCoors(int mx, int my) {
	// gluUnProject() expects double*'s
	static double wcoors[3]   = {0.0, 0.0, 0.0};
	static double viewMat[16] = {0.0};
	static double projMat[16] = {0.0};
	static int    viewport[4] = {0};

	for (int i = 0; i < 16; i += 4) {
		viewMat[i + 0] = viewMatrix.m[i + 0];
		viewMat[i + 1] = viewMatrix.m[i + 1];
		viewMat[i + 2] = viewMatrix.m[i + 2];
		viewMat[i + 3] = viewMatrix.m[i + 3];
		projMat[i + 0] = projMatrix.m[i + 0];
		projMat[i + 1] = projMatrix.m[i + 1];
		projMat[i + 2] = projMatrix.m[i + 2];
		projMat[i + 3] = projMatrix.m[i + 3];
	}

	// glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	// glGetDoublev(GL_MODELVIEW_MATRIX, viewMatrix);
	glGetIntegerv(GL_VIEWPORT, viewport);

	// note: mz value of 0 maps to zNear, mz value of 1 maps to zFar
	// mouse origin is at top-left, OGL window origin is at bottom-left
	float mz = 1.0f;
	int myy = viewport[3] - my;

	glReadPixels(mx, myy,  1, 1,  GL_DEPTH_COMPONENT, GL_FLOAT, &mz);
	gluUnProject(mx, myy, mz,  viewMat, projMat, viewport,  &wcoors[0], &wcoors[1], &wcoors[2]);

	return vec3f(float(wcoors[0]), float(wcoors[1]), float(wcoors[2]));
}






bool FPSCamera::Active() const { return (AUX->GetMouseLook()); }

void FPSCamera::KeyPressed(int key, bool repeat) {
	if (repeat) {
		const float keySens = inputHandler->GetKeySensitivity();
		const bool mouseLook = AUX->GetMouseLook();

		switch (key) {
			case SDLK_LALT:  { VStrafe( 1, keySens); } break;
			case SDLK_LCTRL: { VStrafe(-1, keySens); } break;

			case SDLK_w: { Move(    1, keySens); } break;
			case SDLK_s: { Move(   -1, keySens); } break;
			case SDLK_a: { HStrafe(-1, keySens); } break;
			case SDLK_d: { HStrafe( 1, keySens); } break;
			case SDLK_q: { Roll(   -1, keySens * DEG2RAD(0.5f)); } break;
			case SDLK_e: { Roll(    1, keySens * DEG2RAD(0.5f)); } break;

			case SDLK_UP:    { if (!mouseLook) { Pitch(-1, keySens * DEG2RAD(0.5f)); } } break;
			case SDLK_DOWN:  { if (!mouseLook) { Pitch( 1, keySens * DEG2RAD(0.5f)); } } break;
			case SDLK_LEFT:  { if (!mouseLook) { Yaw(  -1, keySens * DEG2RAD(0.5f)); } } break;
			case SDLK_RIGHT: { if (!mouseLook) { Yaw(   1, keySens * DEG2RAD(0.5f)); } } break;

			default: {} break;
		}
	}
}

void FPSCamera::MousePressed(int b, int, int, bool repeat) {
	if (repeat) {
		if (b == SDL_BUTTON_LEFT)  { Move( 1, inputHandler->GetMouseSensitivity()); }
		if (b == SDL_BUTTON_RIGHT) { Move(-1, inputHandler->GetMouseSensitivity()); }
	}
}
void FPSCamera::MouseMoved(int, int, int dx, int dy) {
	if (AUX->GetMouseLook()) {
		const int xsign =  (dx > 0)?  1: -1;
		const int ysign =  (dy > 0)? -1:  1;
		const float rdx = ((dx > 0)? dx: -dx) / hFOVdeg;
		const float rdy = ((dy > 0)? dy: -dy) / vFOVdeg;

		if (dx != 0) { Yaw(  xsign, rdx); }
		if (dy != 0) { Pitch(ysign, rdy); }
	}
}

void FPSCamera::Yaw(int sign, float sens) {
	const vec3f tmp = vrp + (xdir * sign * sens);

	// keep rotations level in xz-plane
	ydir = YVECf;
	zdir = (tmp - pos).inorm();
	xdir = (zdir.cross(ydir)).inorm();
	vrp  = pos + zdir;

	mat.SetZDir(zdir);
	mat.SetYDir(ydir);
	mat.SetXDir(xdir);
}
void FPSCamera::Pitch(int sign, float sens) {
	const vec3f tmp = vrp + (ydir * sign * sens);

	zdir = (tmp - pos).inorm();
	ydir = (xdir.cross(zdir)).inorm();
	vrp  = pos + zdir;

	mat.SetZDir(zdir);
	mat.SetYDir(ydir);
}
void FPSCamera::Roll(int sign, float sens) {
	ydir += (xdir * sign * sens);
	xdir = (zdir.cross(ydir));

	xdir.inorm();
	ydir.inorm();

	mat.SetXDir(xdir);
	mat.SetYDir(ydir);
}

// forward and backward (along zdir)
void FPSCamera::Move(int sign, float sens) {
	pos += (zdir * sign * sens);
	vrp += (zdir * sign * sens);

	mat.SetPos(pos);
}
// left and right (along xdir)
void FPSCamera::HStrafe(int sign, float sens) {
	pos += (xdir * sign * sens);
	vrp += (xdir * sign * sens);

	mat.SetPos(pos);
}
// up and down (along ydir)
void FPSCamera::VStrafe(int sign, float sens) {
	pos += (ydir * sign * sens);
	vrp += (ydir * sign * sens);

	mat.SetPos(pos);
}

void FPSCamera::RotateX(float angle, bool b) {
	angle = (b? DEG2RAD(angle): angle);

	ydir = YVECf;
	zdir = (zdir * cosf(angle) + ydir * sinf(angle)).norm();
	ydir = (xdir.cross(zdir));
	vrp  = pos + zdir;

	mat.SetZDir(zdir);
	mat.SetYDir(ydir);
}
void FPSCamera::RotateY(float angle, bool b) {
	angle = (b? DEG2RAD(angle): angle);

	zdir = (zdir * cosf(angle) - xdir * sinf(angle)).norm();
	xdir = (zdir.cross(ydir));
	vrp  = pos + zdir;

	mat.SetZDir(zdir);
	mat.SetXDir(xdir);
}
void FPSCamera::RotateZ(float angle, bool b) {
	angle = (b? DEG2RAD(angle): angle);

	xdir = (xdir * cosf(angle) + ydir * sinf(angle)).norm();
	ydir = (xdir.cross(zdir));

	mat.SetXDir(xdir);
	mat.SetYDir(ydir);
}






void OrbitCamera::Init(const vec3f& p, const vec3f& t) {
	const vec3f v = (t - p);
	const vec3f w = v.norm();
	const float d = v.len3D();
	// acosf() takes values in [-1.0, 1.0]
	const float e = RAD2DEG(acosf(v.len2D() / d));
	const float r = RAD2DEG(acosf(w.dot2D(XVECf)));

	// when v is parallel to world z-axis, dot
	// with x-axis will be 0 and rotation then
	// is ambiguous (can be +90 or -90): check
	// the sign of v.z to determine if we are
	// looking down +z or -z
	distance  = cDistance  = d;
	elevation = cElevation = e;
	rotation  = cRotation  = (v.z > 0.0f)? 180.0f + r: 180.0f - r;
	cen       = t;

	active = false;
}

vec3f OrbitCamera::GetOrbitPos() const {
	// returns position on sphere centered at
	// (0, 0, 0) and with radius <distance>
	// from current elevation and rotation
	const float beta = DEG2RAD(elevation);
	const float gamma = DEG2RAD(rotation);

	float cx = distance;
	float cy = 0.0f;
	float cz = 0.0f;
	float tx = cx;

	// elevation around Z
	tx = cx;
	cx = cx * cosf(beta) + cy * sinf(beta);
	cy = tx * sinf(beta) + cy * cosf(beta);

	// rotation around Y
	tx = cx;
	cx = cx * cosf(gamma) - cz * sinf(gamma);
	cz = tx * sinf(gamma) + cz * cosf(gamma);

	return vec3f(cx, cy, cz);
}

void OrbitCamera::KeyPressed(int key, bool) { active = (key == SDLK_SPACE); }
void OrbitCamera::KeyReleased(int key) { active = active && (key != SDLK_SPACE); }

void OrbitCamera::MousePressed(int button, int, int, bool repeat) {
	if (!active) {
		return;
	}

	switch (button) {
		case SDL_BUTTON_WHEELDOWN: {
			distance = std::min(zFarDistance, distance + 100.0f);

			pos = cen - (zdir * distance);
			vrp = pos + zdir;

			mat.SetPos(pos);
		} break;
		case SDL_BUTTON_WHEELUP: {
			distance = std::max(zNearDistance, distance - 100.0f);

			pos = cen - (zdir * distance);
			vrp = pos + zdir;

			mat.SetPos(pos);
		} break;
	}

	if (!repeat) {
		cDistance = distance;
		cRotation = rotation;
		cElevation = elevation;
	}
}

void OrbitCamera::MouseMoved(int x, int y, int dx, int dy) {
	if (!active) {
		return;
	}

	switch (inputHandler->GetLastMouseButton()) {
		case SDL_BUTTON_LEFT: {
			// we want translation wrt. coors of last press
			dx = (inputHandler->GetLastMouseCoors()).x - x;
			dy = (inputHandler->GetLastMouseCoors()).y - y;

			// orbit change does not affect distance
			rotation  = cRotation  - (dx * 0.25f);
			elevation = cElevation - (dy * 0.25f);
		} break;
		case SDL_BUTTON_RIGHT: {
			dx = (inputHandler->GetLastMouseCoors()).x - x;
			dy = (inputHandler->GetLastMouseCoors()).y - y;

			// distance change does not affect orbit
			distance = cDistance - (dy * 0.5f * 10.0f);
		} break;
	}

	if (elevation >  89.0f) elevation =  89.0f;
	if (elevation < -89.0f) elevation = -89.0f;
	if (distance  <   1.0f) distance  =   1.0f;

	switch (inputHandler->GetLastMouseButton()) {
		case SDL_BUTTON_LEFT: {
			pos  = cen + GetOrbitPos();
			vrp  = pos + ((cen - pos).inorm());
			ydir = YVECf;

			// keep rotations level in xz-plane
			mat.SetPos(pos);
			mat.SetYDir(ydir);
		} break;

		case SDL_BUTTON_RIGHT: {
			pos = cen - (zdir * distance);
			vrp = pos + zdir;

			mat.SetPos(pos);
		} break;

		case SDL_BUTTON_MIDDLE: {
			const int xsign = (dx > 0)?  1:  1;
			const int ysign = (dy > 0)? -1: -1;
			// const float rdx = abs(dx) / hFOVdeg;
			// const float rdy = abs(dy) / vFOVdeg;
			HStrafe(xsign, dx * 2);
			VStrafe(ysign, dy * 2);
		} break;
	}
}

void OrbitCamera::HStrafe(int sign, float sens) {
	pos += (xdir * sign * sens);
	vrp += (xdir * sign * sens);
	cen += (xdir * sign * sens);

	mat.SetPos(pos);
}
void OrbitCamera::VStrafe(int sign, float sens) {
	pos += (ydir * sign * sens);
	vrp += (ydir * sign * sens);
	cen += (ydir * sign * sens);

	mat.SetPos(pos);
}


void OverheadCamera::Init(const vec3f&, const vec3f&) {
	sensMultiplier = 1.0f;
}

void OverheadCamera::KeyPressed(int key, bool) {
	const float scrollSpeed = inputHandler->GetKeySensitivity() * 2.0f * sensMultiplier;

	if (key == SDLK_LSHIFT || key == SDLK_RSHIFT)
		shiftPressed = true;

	if (key == SDLK_LCTRL  || key == SDLK_RCTRL)
		ctrlPressed = true;

	if (shiftPressed)
		sensMultiplier = 5.0f;

	switch (key) {
		case SDLK_w: { ScrollNorthSouth(-1, scrollSpeed); } break;
		case SDLK_s: { ScrollNorthSouth( 1, scrollSpeed); } break;
		case SDLK_a: { ScrollEastWest(  -1, scrollSpeed); } break;
		case SDLK_d: { ScrollEastWest(   1, scrollSpeed); } break;
	}
}
void OverheadCamera::KeyReleased(int key) {
	if (key == SDLK_LSHIFT || key == SDLK_RSHIFT)
		shiftPressed = false;

	if (key == SDLK_LCTRL || key == SDLK_RCTRL)
		ctrlPressed = false;

	if (!shiftPressed)
		sensMultiplier = 1.0f;
}
void OverheadCamera::MousePressed(int button, int, int, bool) {
	const float zoomSpeed = inputHandler->GetKeySensitivity() * 10.0f * sensMultiplier;

	switch (button) {
		case SDL_BUTTON_WHEELDOWN: {
			if (ctrlPressed)
				Rotate(-5.0f);
			else
				Zoom( 1, zoomSpeed); 
		} break;
		case SDL_BUTTON_WHEELUP:   { 
			if (ctrlPressed)
				Rotate( 5.0f);
			else
				Zoom(-1, zoomSpeed);
		} break;
	}
}

void OverheadCamera::ScrollNorthSouth(int sign, float sens) {
	// translate in world-space
	pos += (ZVECf * sign * sens);
	vrp += (ZVECf * sign * sens);
	tar += (ZVECf * sign * sens);

	mat.SetPos(pos);
}
void OverheadCamera::ScrollEastWest(int sign, float sens) {
	// translate in world-space
	pos += (XVECf * sign * sens);
	vrp += (XVECf * sign * sens);
	tar += (XVECf * sign * sens);

	mat.SetPos(pos);
}
void OverheadCamera::Zoom(int sign, float sens) {
	pos += (zdir * sign * sens);
	vrp += (zdir * sign * sens);

	mat.SetPos(pos);
}
void OverheadCamera::Rotate(float alpha) {

	vec3f tmp = pos - tar;

	alpha = DEG2RAD(alpha);
	tmp   = tmp.rotateX(alpha);
	zdir  = -tmp.norm();
	ydir  = (xdir.cross(zdir));
	pos   = tmp + tar;
	vrp   = pos + zdir;

	mat.SetZDir(zdir);
	mat.SetYDir(ydir);
	mat.SetPos(pos);
}
