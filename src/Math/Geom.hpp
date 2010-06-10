#ifndef PFFG_GEOM_HDR
#define PFFG_GEOM_HDR

#include "./vec3fwd.hpp"
#include "./vec3.hpp"

namespace geom {
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

	// returns the absolute (!) orthogonal distance from <p> to segment <AB>
	float PointLineDistance(const vec3f& A, const vec3f& B, const vec3f& p) {
		return (((p - A).cross(p - B)).len3D() / (B - A).len3D());
	}


	// <v> must not be equal to either <x1> or <x2> (which form the edge)
	bool PointSameSideEdge(const vec3f& pnt, const vec3f& vrt,  const vec3f& x1, const vec3f& x2) {
		const vec3f px1 = pnt - x1;
		const vec3f vx1 = vrt - x1;
		const vec3f e   = x2  - x1;
		const vec3f v0 = e.cross(px1);
		const vec3f v1 = e.cross(vx1);
		return (v0.dot3D(v1) >= 0.0f);
	}

	// p is in the triangle <A, B, C> if
	//   1) p is on the "same side" of edge BC as vertex A
	//   2) p is on the "same side" of edge CA as vertex B
	//   3) p is on the "same side" of edge AB as vertex C
	bool PointInTriangle(const vec3f& A, const vec3f& B, const vec3f& C, const vec3f& p) {
		return (PointSameSideEdge(p, A,  B, C) && PointSameSideEdge(p, B,  A, C) && PointSameSideEdge(p, C,  A, B));
	}
}

#endif
