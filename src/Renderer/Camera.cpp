#include <GL/gl.h>
#include <SDL/SDL.h>

#include "./Camera.hpp"
#include "../Input/InputHandler.hpp"
#include "../System/EngineAux.hpp"

#define SDL_BUTTON_WHEEL 8

Camera::Camera(const vec3f& p, const vec3f& t, CAMERA_MOVE_MODE mm, CAMERA_PROJ_MODE pm):
	mat(p, XVECf, YVECf, ZVECf),
	vrp(t),
	pos(p),
	xdir(mat.GetXDir()),
	ydir(mat.GetYDir()),
	zdir(mat.GetZDir()),
	moveMode(mm),
	projMode(pm) {

	inputHandler->AddReceiver(this);
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
	const int vppx = WIN->GetViewPortPosX();
	const int vpsx = WIN->GetViewPortSizeX();
	const int vpsy = WIN->GetViewPortSizeY();

	const float dx = ((x - vppx - vpsx * 0.5f) / vpsy) * (thvFOVrad * 2.0f);
	const float dy = ((y - vpsy        * 0.5f) / vpsy) * (thvFOVrad * 2.0f);

	return ((zdir - (ydir * dy) + (xdir * dx)).norm());
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
	const vec3f zdirY = (-zdir * (    thvFOVrad           ));
	const vec3f zdirX = (-zdir * (tanf(hvFOVrad * hAspRat)));

	frustumT = (zdirY + ydir).inorm();
	frustumB = (zdirY - ydir).inorm();
	frustumR = (zdirX + xdir).inorm();
	frustumL = (zdirX - xdir).inorm();
}

void Camera::Clamp() {
	/*
	const float dx0 = pos.x - MAXVEC.x;
	const float dx1 = pos.x - MINVEC.x;
	const float dy0 = pos.y - MAXVEC.y;
	const float dy1 = pos.y - MINVEC.y;
	const float dz0 = pos.z - MAXVEC.z;
	const float dz1 = pos.z - MINVEC.z;
	if (dx0 > 0.0f) { pos.x -= dx0; vrp.x -= dx0; }
	if (dx1 < 0.0f) { pos.x -= dx1; vrp.x -= dx1; }
	if (dy0 > 0.0f) { pos.y -= dy0; vrp.y -= dy0; }
	if (dy1 < 0.0f) { pos.y -= dy1; vrp.y -= dy1; }
	if (dz0 > 0.0f) { pos.z -= dz0; vrp.z -= dz0; }
	if (dz1 < 0.0f) { pos.z -= dz1; vrp.z -= dz1; }
	*/
}

void Camera::SetState(const Camera* c) {
	pos    = c->pos;
	xdir   = c->xdir;
	ydir   = c->ydir;
	zdir   = c->zdir;
	vrp    = c->vrp;
	mat    = c->mat;

	hFOVdeg   = c->hFOVdeg;
	hhFOVrad  = c->hhFOVrad;
	thhFOVrad = c->thhFOVrad;
	vFOVdeg   = c->vFOVdeg;
	hvFOVrad  = c->hvFOVrad;
	thvFOVrad = c->thvFOVrad;

	hAspRat = c->hAspRat;
	zNear   = c->zNear;
	zFar    = c->zFar;
}

void Camera::Update() {
	UpdateCoorSys();
	UpdateFrustum();
	Clamp();
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
	const float t = zNear * thvFOVrad;
	const float b = -t;
	const float l = b * hAspRat;
	const float r = t * hAspRat;

	projMatrix.m[ 0] = (2.0f * zNear) / (r - l);
	projMatrix.m[ 1] =  0.0f;
	projMatrix.m[ 2] =  0.0f;
	projMatrix.m[ 3] =  0.0f;

	projMatrix.m[ 4] =  0.0f;
	projMatrix.m[ 5] = (2.0f * zNear) / (t - b);
	projMatrix.m[ 6] =  0.0f;
	projMatrix.m[ 7] =  0.0f;

	projMatrix.m[ 8] = (r + l) / (r - l);
	projMatrix.m[ 9] = (t + b) / (t - b);
	projMatrix.m[10] = -(zFar + zNear) / (zFar - zNear);
	projMatrix.m[11] = -1.0f;

	projMatrix.m[12] =   0.0f;
	projMatrix.m[13] =   0.0f;
	projMatrix.m[14] = -(2.0f * zFar * zNear) / (zFar - zNear);
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
	const float t =  1.0f * WIN->GetViewPortSizeY();
	const float b = -1.0f * WIN->GetViewPortSizeY();
	const float l = -1.0f * WIN->GetViewPortSizeX();
	const float r =  1.0f * WIN->GetViewPortSizeX();

	const float tx = -((r + l) / (r - l));
	const float ty = -((t + b) / (t - b));
	const float tz = -((zFar + zNear) / (zFar - zNear));

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
	projMatrix.m[10] = -2.0f / (zFar - zNear);
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






void FPSCamera::KeyPressed(int key, bool repeat) {
	if (repeat) {
		if (key == SDLK_LALT)  { VStrafe( 1, INP->GetKeySens() * 2.000f); }
		if (key == SDLK_LCTRL) { VStrafe(-1, INP->GetKeySens() * 2.000f); }

		if (key == SDLK_w) { Move(    1, INP->GetKeySens() * 2.000f); }
		if (key == SDLK_s) { Move(   -1, INP->GetKeySens() * 2.000f); }
		if (key == SDLK_a) { HStrafe(-1, INP->GetKeySens() * 2.000f); }
		if (key == SDLK_d) { HStrafe( 1, INP->GetKeySens() * 2.000f); }
		if (key == SDLK_q) { Roll(   -1, INP->GetKeySens() * 0.005f); }
		if (key == SDLK_e) { Roll(    1, INP->GetKeySens() * 0.005f); }

		if (!ENG->GetMouseLook()) {
			if (key == SDLK_UP   ) { Pitch(-1, INP->GetKeySens() * 0.005f); }
			if (key == SDLK_DOWN ) { Pitch( 1, INP->GetKeySens() * 0.005f); }
			if (key == SDLK_LEFT ) { Yaw(  -1, INP->GetKeySens() * 0.005f); }
			if (key == SDLK_RIGHT) { Yaw(   1, INP->GetKeySens() * 0.005f); }
		}
	}
}

void FPSCamera::MousePressed(int b, int, int, bool repeat) {
	if (repeat) {
		if (b == SDL_BUTTON_LEFT)  { Move( 1, INP->GetMouseSens()); }
		if (b == SDL_BUTTON_RIGHT) { Move(-1, INP->GetMouseSens()); }
	}
}
void FPSCamera::MouseMoved(int, int, int dx, int dy) {
	if (ENG->GetMouseLook()) {
		const int xsign =  (dx > 0)?  1: -1;
		const int ysign =  (dy > 0)? -1:  1;
		const float rdx = ((dx > 0)? dx: -dx) / hFOVdeg;
		const float rdy = ((dy > 0)? dy: -dy) / vFOVdeg;

		if (dx != 0) { Yaw(  xsign, rdx); }
		if (dy != 0) { Pitch(ysign, rdy); }
	}
}

void FPSCamera::Yaw(int sign, float sens) {
	const vec3f tmp = vrp + (xdir * sign * sens * 0.5f);

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
	const vec3f tmp = vrp + (ydir * sign * sens * 0.5f);

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

	// rotate around Z (elevation)
	tx = cx;
	cx = cx * cosf(beta) + cy * sinf(beta);
	cy = tx * sinf(beta) + cy * cosf(beta);

	// rotate around Y (rotation)
	tx = cx;
	cx = cx * cosf(gamma) - cz * sinf(gamma);
	cz = tx * sinf(gamma) + cz * cosf(gamma);

	return vec3f(cx, cy, cz);
}

void OrbitCamera::MousePressed(int, int, int, bool repeat) {
	if (!repeat) {
		cDistance = distance;
		cRotation = rotation;
		cElevation = elevation;
	}
}
void OrbitCamera::MouseMoved(int x, int y, int dx, int dy) {
	switch (INP->GetLastMouseButton()) {
		case SDL_BUTTON_LEFT: {
			// we want translation wrt. coors of last press
			dx = INP->GetLastMouseX() - x;
			dy = INP->GetLastMouseY() - y;

			// orbit change does not affect distance
			rotation  = cRotation  - (dx * 0.25f);
			elevation = cElevation - (dy * 0.25f);
		} break;
		case SDL_BUTTON_RIGHT: {
			dx = INP->GetLastMouseX() - x;
			dy = INP->GetLastMouseY() - y;

			// distance change does not affect orbit
			distance = cDistance - (dy * 0.5f * 10.0f);
		} break;
	}

	if (elevation >  89.0f) elevation =  89.0f;
	if (elevation < -89.0f) elevation = -89.0f;
	if (distance  <   1.0f) distance  =   1.0f;

	switch (INP->GetLastMouseButton()) {
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
		case SDL_BUTTON_WHEEL:
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
