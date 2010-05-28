#ifndef PFFG_GEOM_HDR
#define PFFG_GEOM_HDR

#include "./vec3fwd.hpp"
#include "./vec3.hpp"

struct Plane {
public:
	Plane(): n(NVECf), d(0.0f) {
	}
	Plane(const vec3f& abc, float dst): n(abc.norm()), d(dst) {
		p = d / abc.len3D();
	}

	float PointDistance(const vec3f& point) const {
		return (n.dot3D(point) - p);
	}

	const vec3f& GetNormal() const { return n; }
	float GetDistance() const { return d; }

private:
	vec3f n;
	float d;
	float p;
};

#endif
