#include <algorithm>
#include <cassert>
#include <cmath>

#include "./Ground.hpp"
#include "./ReadMap.hpp"
#include "../Math/vec3.hpp"

CGround* CGround::GetInstance() {
	static CGround* g = NULL;
	static unsigned int depth = 0;

	if (g == NULL) {
		assert(depth == 0);

		depth += 1;
		g = new CGround();
		depth -= 1;
	}

	return g;
}

void CGround::FreeInstance(CGround* g) {
	delete g;
}



float CGround::LineGroundCol(vec3f from, vec3f to) {
	float savedLength = 0.0f;

	if (from.z > readMap->maxzpos && to.z < readMap->maxzpos) {
		// special case: origin outside map, destination inside
		const vec3f dir = (to - from).norm();

		savedLength = -(from.z - readMap->maxzpos) / dir.z;
		from += dir * savedLength;
	}

	if (from.x <             0.0f) { from.x =             0.0f; }
	if (from.x > readMap->maxxpos) { from.x = readMap->maxxpos; }
	if (from.z <             0.0f) { from.z =             0.0f; }
	if (from.z > readMap->maxzpos) { from.z = readMap->maxzpos; }

	vec3f dir = to - from;
	float maxLength = dir.len3D();
	dir /= maxLength;

	if (from.x + dir.x * maxLength < 1.0f)
		maxLength = (1.0f - from.x) / dir.x;
	else if (from.x + dir.x * maxLength > readMap->maxxpos)
		maxLength = (readMap->maxxpos - from.x) / dir.x;

	if (from.z + dir.z * maxLength < 1.0f)
		maxLength = (1.0f - from.z) / dir.z;
	else if (from.z + dir.z * maxLength > readMap->maxzpos)
		maxLength = (readMap->maxzpos - from.z) / dir.z;

	to = from + dir * maxLength;

	const float dx = to.x - from.x;
	const float dz = to.z - from.z;
	float xp = from.x;
	float zp = from.z;
	float ret;
	float xn, zn;

	bool keepgoing = true;
	const int sqSz = readMap->SQUARE_SIZE;

	if ((floor(from.x / sqSz) == floor(to.x / sqSz)) && (floor(from.z / sqSz) == floor(to.z / sqSz))) {
		ret = LineGroundSquareCol(from, to, int(floor(from.x / sqSz)), int(floor(from.z / sqSz)));

		if (ret >= 0.0f) {
			return ret;
		}
	} else if (floor(from.x / sqSz) == floor(to.x / sqSz)) {
		while (keepgoing) {
			ret = LineGroundSquareCol(from, to, (int) floor(xp / sqSz), (int) floor(zp / sqSz));
			if (ret >= 0.0f) {
				return ret + savedLength;
			}

			keepgoing = (fabs(zp - from.z) < fabs(dz));

			if (dz > 0.0f) {
				zp += sqSz;
			} else {
				zp -= sqSz;
			}
		}
	} else if (floor(from.z / sqSz) == floor(to.z / sqSz)) {
		while (keepgoing) {
			ret = LineGroundSquareCol(from, to, (int) floor(xp / sqSz), (int) floor(zp / sqSz));
			if (ret >= 0.0f) {
				return ret + savedLength;
			}

			keepgoing = (fabs(xp - from.x) < fabs(dx));

			if (dx > 0.0f) {
				xp += sqSz;
			} else {
				xp -= sqSz;
			}
		}
	} else {
		while (keepgoing) {
			float xs, zs;

			// Push value just over the edge of the square
			// This is the best accuracy we can get with floats:
			// add one digit and (xp*constant) reduces to xp itself
			// This accuracy means that at (16384,16384) (lower right of 32x32 map)
			// 1 in every 1/(16384*1e-7f/8)=4883 clicks on the map will be ignored.
			if (dx > 0.0f) xs = floor(xp * 1.0000001f / sqSz);
			else           xs = floor(xp * 0.9999999f / sqSz);
			if (dz > 0.0f) zs = floor(zp * 1.0000001f / sqSz);
			else           zs = floor(zp * 0.9999999f / sqSz);

			ret = LineGroundSquareCol(from, to, (int) xs, (int) zs);
			if (ret >= 0.0f) {
				return ret + savedLength;
			}

			keepgoing = (fabs(xp - from.x) < fabs(dx) && fabs(zp - from.z) < fabs(dz));

			if (dx > 0.0f) {
				// distance xp to right edge of square (xs,zs) divided by dx, xp += xn*dx puts xp on the right edge
				xn = (xs * sqSz + sqSz - xp) / dx;
			} else {
				// distance xp to left edge of square (xs,zs) divided by dx, xp += xn*dx puts xp on the left edge
				xn = (xs * sqSz - xp) / dx;
			}
			if (dz > 0.0f) {
				// distance zp to bottom edge of square (xs,zs) divided by dz, zp += zn*dz puts zp on the bottom edge
				zn = (zs * sqSz + sqSz - zp) / dz;
			} else {
				// distance zp to top edge of square (xs,zs) divided by dz, zp += zn*dz puts zp on the top edge
				zn = (zs * sqSz - zp) / dz;
			}

			// xn and zn are always positive, minus signs are divided out above

			// this puts (xp,zp) exactly on the first edge you see if you look from (xp,zp) in the (dx,dz) direction
			if (xn < zn) {
				xp += xn * dx;
				zp += xn * dz;
			} else {
				xp += zn * dx;
				zp += zn * dz;
			}
		}
	}

	return -1.0f;
}

float CGround::LineGroundSquareCol(const vec3f& from, const vec3f& to, int xs, int ys) {
	if ((xs < 0) || (ys < 0) || (xs >= readMap->mapx - 1) || (ys >= readMap->mapy - 1))
		return -1;

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
	else if (xsquare > readMap->mapx - 1)
		xsquare = readMap->mapx - 1;
	if (ysquare < 0)
		ysquare = 0;
	else if (ysquare > readMap->mapy - 1)
		ysquare = readMap->mapy - 1;

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

// note: not called for SMF rendering purposes (see SMFReadMap::GetLightValue())
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

// note: not called for SMF rendering purposes (see SMFReadMap::GetLightValue())
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

float CGround::TrajectoryGroundCol(const vec3f& from, const vec3f& flatdir, float length, float linear, float quadratic) {
	/// from.CheckInBounds();

	vec3f dir(flatdir.x, linear, flatdir.z);

	for (float l = 0.0f; l < length; l += 8.0f) {
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
		std::max(0, std::min(readMap->mapx - 1, (int(pos.x) / readMap->SQUARE_SIZE))) +
		std::max(0, std::min(readMap->mapy - 1, (int(pos.z) / readMap->SQUARE_SIZE))) * readMap->mapx;
};
