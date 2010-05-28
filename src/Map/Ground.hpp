#ifndef PFFG_GROUND_HDR
#define PFFG_GROUND_HDR

#include "../Math/vec3fwd.hpp"

class CGround {
public:
	static CGround* GetInstance();
	static void FreeInstance(CGround*);

	float GetApproximateHeight(float x, float y);
	float GetSlope(float x, float y);
	float GetHeight(float x, float y);
	float GetHeight2(float x, float y);
	float GetOrigHeight(float x, float y);
	vec3f& GetNormal(float x, float y);
	vec3f GetSmoothNormal(float x, float y);
	float LineGroundCol(vec3f from, vec3f to);
	float TrajectoryGroundCol(const vec3f&, const vec3f&, float, float, float);
	int GetSquare(const vec3f&);

private:
	float LineGroundSquareCol(const vec3f& from, const vec3f& to, int xs, int ys);
};

#define ground (CGround::GetInstance())

#endif
