#include <algorithm>
#include <cmath>

#include "./Ground.hpp"
#include "./ReadMap.hpp"
#include "../Math/vec3.hpp"
#include "../System/Debugger.hpp"

CGround* CGround::GetInstance() {
	static CGround* g = NULL;
	static unsigned int depth = 0;

	if (g == NULL) {
		PFFG_ASSERT(depth == 0);

		depth += 1;
		g = new CGround();
		depth -= 1;
	}

	return g;
}

void CGround::FreeInstance(CGround* g) {
	delete g;
}



float CGround::LineGroundCol(const vec3f& from, const vec3f& to) {
	const float xmax = readMap->maxxpos;
	const float zmax = readMap->maxzpos;
	const int maxIdx = ((readMap->mapx + 1) * (readMap->mapy + 1)) - 1;
	const float* hm = readMap->GetHeightmap();

	// test whether the ray can intersect at all
	if (from.x < 0.0f && to.x < 0.0f) { return -1.0f; }
	if (from.z < 0.0f && to.z < 0.0f) { return -1.0f; }

	if (from.x > xmax && to.x > xmax) { return -1.0f; }
	if (from.z > zmax && to.z > zmax) { return -1.0f; }

	if (from.y < readMap->currMinHeight && to.y < readMap->currMinHeight) { return -1.0f; }
	if (from.y > readMap->currMaxHeight && to.y > readMap->currMaxHeight) { return -1.0f; }

	// segment (squared) length and direction
	const float len = (to - from).sqLen3D();
	const vec3f dir = (to - from).norm();

	// position being stepped, (squared) distance travelled
	vec3f pos = from;
	float dst = 0.0f;

	// test whether the ray can potentially intersect
	// (if so, adjust the segment's starting position
	// to the nearest map-edge)
	while (!readMap->PosInBounds(pos)) {
		if (pos.x < 0.0f) {
			if (dir.x <= 0.0f) {
				return -1.0f;
			} else {
				dst = -pos.x / dir.x;            // distance to x=0 line
				pos += (dir * dst);
			}
		}
		else if (pos.x > xmax) {
			if (dir.x >= 0.0f) {
				return -1.0f;
			} else {
				dst = (pos.x - xmax) / -dir.x;   // distance to x=xmax line
				pos += (dir * dst);
			}
		}

		if (pos.z < 0.0f) {
			if (dir.z <= 0.0f) {
				return -1.0f;
			} else {
				dst = -pos.z / dir.z;            // distance to z=0 line
				pos += (dir * dst);
			}
		}
		else if (pos.z > zmax) {
			if (dir.z >= 0.0f) {
				return -1.0f;
			} else {
				dst = (pos.z - zmax) / -dir.z;   // distance to z=zmax line
				pos += (dir * dst);
			}
		}

		// reset
		dst = 0.0f;
	}


	bool haveIntersection = false;
	// heightmap-index of current position
	int idx = readMap->pos2square(pos);

	// <pos> is guaranteed to be in-bounds at this
	// point, so the index should never be illegal
	PFFG_ASSERT(idx >= 0 && idx <= maxIdx);

	// if, after shifting the segment start-position to the
	// edge of the map, we are already below the terrain at
	// that point, no intersection can happen
	if (pos.y < hm[idx]) {
		return -1.0f;
	}

	while (!haveIntersection) {
		if (dst > len) { break; }
		if (idx < 0 || idx > maxIdx) { break; }
		if (pos.x < 0.0f || pos.x > xmax) { break; }
		if (pos.z < 0.0f || pos.z > zmax) { break; }

		if (pos.y <= hm[idx]) {
			haveIntersection = true;
		} else {
			pos += (dir * readMap->SQUARE_SIZE);
			dst = (pos - from).sqLen3D();
			idx = readMap->pos2square(pos);
		}
	}

	return ((haveIntersection)? fastmath::sqrt2(dst): -1.0f);
}



float CGround::LineGroundSquareCol(const vec3f& from, const vec3f& to, int xs, int ys) {
	if ((xs < 0) || (ys < 0) || (xs > readMap->maxxpos) || (ys > readMap->maxzpos)) {
		return -1;
	}

	vec3f dir = to - from;
	vec3f tri;

	const int sqSz = readMap->SQUARE_SIZE;

	// triangle 1
	tri.x = xs * sqSz;
	tri.z = ys * sqSz;
	const float* heightmap = readMap->GetHeightmap();
	tri.y = heightmap[ys * (readMap->mapx + 1) + xs];

	vec3f norm = readMap->facenormals[(ys * readMap->mapx + xs) * 2];
	float side1 = (from - tri).dot3D(norm);
	float side2 = (to - tri).dot3D(norm);

	if (side2 <= 0.0f) {
		float dif = side1 - side2;
		if (dif != 0.0f) {
			float frontpart = side1 / dif;
			vec3f col = from + (dir * frontpart);

			if ((col.x >= tri.x) && (col.z >= tri.z) && (col.x + col.z <= tri.x + tri.z + sqSz)) {
				return ((col - from).len3D());
			}
		}
	}

	// triangle 2
	tri.x = (xs + 1) * sqSz;
	tri.z = (ys + 1) * sqSz;
	tri.y = heightmap[(ys + 1) * (readMap->mapx + 1) + xs + 1];

	norm = readMap->facenormals[(ys * readMap->mapx + xs) * 2 + 1];
	side1 = (from - tri).dot3D(norm);
	side2 = (to - tri).dot3D(norm);

	if (side2 <= 0.0f) {
		float dif = side1 - side2;
		if (dif != 0.0f) {
			float frontpart = side1 / dif;
			vec3f col = from + (dir * frontpart);

			if ((col.x <= tri.x) && (col.z <= tri.z) && (col.x + col.z >= tri.x + tri.z - sqSz)) {
				return ((col - from).len3D());
			}
		}
	}
	return -2;
}

float CGround::GetApproximateHeight(float x, float y) {
	int xsquare = int(x) / readMap->SQUARE_SIZE;
	int ysquare = int(y) / readMap->SQUARE_SIZE;

	if (xsquare < 0)
		xsquare = 0;
	else if (xsquare > readMap->maxxpos)
		xsquare = readMap->maxxpos;

	if (ysquare < 0)
		ysquare = 0;
	else if (ysquare > readMap->maxzpos)
		ysquare = readMap->maxzpos;

	return readMap->centerheightmap[xsquare + ysquare * readMap->mapx];
}

float CGround::GetHeight(float x, float y) {
	if (x < 1.0f)
		x = 1.0f;
	else if (x > readMap->maxxpos)
		x = readMap->maxxpos;

	if (y < 1.0f)
		y = 1.0f;
	else if (y > readMap->maxzpos)
		y = readMap->maxzpos;

	float r = 0.0f;
	const int sqSz = readMap->SQUARE_SIZE;
	int sx = (int) (x / sqSz);
	int sy = (int) (y / sqSz);
	float dx = (x - sx * sqSz) * (1.0f / sqSz);
	float dy = (y - sy * sqSz) * (1.0f / sqSz);
	int hs = sx + sy * (readMap->mapx + 1);
	const float* heightmap = readMap->GetHeightmap();

	if (dx + dy < 1.0f) {
		float xdif = (dx) * (heightmap[hs +                 1] - heightmap[hs]);
		float ydif = (dy) * (heightmap[hs + readMap->mapx + 1] - heightmap[hs]);
		r = heightmap[hs] + xdif + ydif;
	} else {
		float xdif = (1.0f - dx) * (heightmap[hs + readMap->mapx + 1] - heightmap[hs + 1 + 1 + readMap->mapx]);
		float ydif = (1.0f - dy) * (heightmap[hs                 + 1] - heightmap[hs + 1 + 1 + readMap->mapx]);
		r = heightmap[hs + 1 + 1 + readMap->mapx] + xdif + ydif;
	}

	return r;
}

float CGround::GetOrigHeight(float x, float y) {
	if (x < 1.0f)
		x = 1.0f;
	else if (x > readMap->maxxpos)
		x = readMap->maxxpos;

	if (y < 1.0f)
		y = 1.0f;
	else if (y > readMap->maxzpos)
		y = readMap->maxzpos;

	float r = 0.0f;
	const int sqSz = readMap->SQUARE_SIZE;
	int sx = (int) (x / sqSz);
	int sy = (int) (y / sqSz);
	float dx = (x - sx * sqSz) * (1.0f / sqSz);
	float dy = (y - sy * sqSz) * (1.0f / sqSz);
	int hs = sx + sy * (readMap->mapx + 1);
	const std::vector<float>& orgheightmap = readMap->orgheightmap;

	if (dx + dy < 1.0f) {
		float xdif = (dx) * (orgheightmap[hs +                 1] - orgheightmap[hs]);
		float ydif = (dy) * (orgheightmap[hs + readMap->mapx + 1] - orgheightmap[hs]);
		r = orgheightmap[hs] + xdif + ydif;
	} else {
		float xdif = (1.0f - dx) * (orgheightmap[hs + readMap->mapx + 1] - orgheightmap[hs + 1 + 1 + readMap->mapx]);
		float ydif = (1.0f - dy) * (orgheightmap[hs +                 1] - orgheightmap[hs + 1 + 1 + readMap->mapx]);
		r = orgheightmap[hs + 1 + 1 + readMap->mapx] + xdif + ydif;
	}

	return r;
}

// note: not called for rendering purposes (see SMFReadMap::GetLightValue())
vec3f& CGround::GetNormal(float x, float y) {
	if (x < 1.0f)
		x = 1.0f;
	else if (x > readMap->maxxpos)
		x = readMap->maxxpos;

	if (y < 1.0f)
		y = 1.0f;
	else if (y > readMap->maxzpos)
		y = readMap->maxzpos;

	const int sqSz = readMap->SQUARE_SIZE;

	return (readMap->facenormals[(int(x) / sqSz + int(y) / sqSz * readMap->mapx) * 2]);
}

float CGround::GetSlope(float x, float y) {
	if (x < 1.0f)
		x = 1.0f;
	else if (x > readMap->maxxpos)
		x = readMap->maxxpos;

	if (y < 1.0f)
		y = 1.0f;
	else if (y > readMap->maxzpos)
		y = readMap->maxzpos;

	const int sqSz = readMap->SQUARE_SIZE;

	return (1.0f - readMap->facenormals[(int(x) / sqSz + int(y) / sqSz * readMap->mapx) * 2].y);
}

// note: not called for rendering purposes (see SMFReadMap::GetLightValue())
vec3f CGround::GetSmoothNormal(float x, float y) {
	const int sqSz = readMap->SQUARE_SIZE;

	int sx = (int) floor(x / sqSz);
	int sy = (int) floor(y / sqSz);

	if (sy < 1) sy = 1;
	if (sx < 1) sx = 1;
	if (sy >= readMap->mapy - 1) sy = readMap->mapy - 2;
	if (sx >= readMap->mapx - 1) sx = readMap->mapx - 2;

	float dx = (x - sx * sqSz) / sqSz;
	float dy = (y - sy * sqSz) / sqSz;

	int sy2;
	float fy;

	if (dy > 0.5f) {
		sy2 = sy + 1;
		fy = dy - 0.5f;
	} else {
		sy2 = sy - 1;
		fy = 0.5f - dy;
	}

	int sx2;
	float fx;

	if (dx > 0.5f) {
		sx2 = sx + 1;
		fx = dx - 0.5f;
	} else {
		sx2 = sx - 1;
		fx = 0.5f - dx;
	}

	float ify = 1.0f - fy;
	float ifx = 1.0f - fx;

	vec3f n1 = (readMap->facenormals[ (sy   * readMap->mapx + sx ) * 2] + readMap->facenormals[(sy  * readMap->mapx + sx ) * 2 + 1]) * ifx * ify;
	vec3f n2 = (readMap->facenormals[ (sy   * readMap->mapx + sx2) * 2] + readMap->facenormals[(sy  * readMap->mapx + sx2) * 2 + 1]) *  fx * ify;
	vec3f n3 = (readMap->facenormals[((sy2) * readMap->mapx + sx ) * 2] + readMap->facenormals[(sy2 * readMap->mapx + sx ) * 2 + 1]) * ifx * fy;
	vec3f n4 = (readMap->facenormals[((sy2) * readMap->mapx + sx2) * 2] + readMap->facenormals[(sy2 * readMap->mapx + sx2) * 2 + 1]) *  fx * fy;

	vec3f norm1 = n1 + n2 + n3 + n4;
	norm1.inorm();

	return norm1;
}

float CGround::TrajectoryGroundCol(vec3f& from, const vec3f& flatdir, float length, float linear, float quadratic) {
	readMap->SetPosInBounds(from);

	vec3f dir(flatdir.x, linear, flatdir.z);

	for (float l = 0.0f; l < length; l += readMap->SQUARE_SIZE) {
		vec3f pos(from + dir * l);
		pos.y += quadratic * l * l;

		if (GetApproximateHeight(pos.x, pos.z) > pos.y) {
			return l;
		}
	}

	return -1;
}

inline int GetSquare(const vec3f& pos) {
	return
		std::max(0, std::min(readMap->maxxpos, (int(pos.x) / readMap->SQUARE_SIZE))) +
		std::max(0, std::min(readMap->maxzpos, (int(pos.z) / readMap->SQUARE_SIZE))) * readMap->mapx;
};
