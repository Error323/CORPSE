#include <cassert>
#include <GL/gl.h>

#include "./RenderThread.hpp"
#include "./Camera.hpp"
#include "./SMFRenderer.hpp"
#include "../Map/ReadMap.hpp"
#include "../Map/SMF/SMFReadMap.hpp"
#include "../Math/BitOps.hpp"
#include "../System/EngineAux.hpp"
#include "../System/Logger.hpp"

typedef CSMFRenderer::Square RSquare;

CSMFRenderer::CSMFRenderer() {
	BuildSquaresArray();
}

CSMFRenderer::~CSMFRenderer() {
	for (int i = 0; i < numSquares; i++) {
		if (squares[i].hasDisLst) {
			glDeleteLists(squares[i].disLstID, 1);
		}
	}
}

void CSMFRenderer::BuildSquaresArray() {
	const int mx = int(readMap->maxxpos + 1), mxSq = mx * mx;
	const int mz = int(readMap->maxzpos + 1), mzSq = mz * mz;

	viewRadiusSq = mxSq + mzSq;
	viewRadius = int(sqrtf(viewRadiusSq));

	// quadSize{X, Z} are in world-space units
	squareSizeX = 64;
	squareSizeZ = 64;
	// numSquares{X, Z} are in world-space
	numSquaresX = int((readMap->maxxpos + 1) / squareSizeX);
	numSquaresZ = int((readMap->maxzpos + 1) / squareSizeZ);
	numSquares = numSquaresX * numSquaresZ;

	// the maximum number of vertices per big square is given by
	// (squareSizeX + 1) * (squareSizeZ + 1) at LOD 1 (size <s>
	// means <s + 1> vertices in that direction), the number of
	// quad "tiles" per square by (squareSizeX * squareSizeZ)
	//
	// NxM grid (on graph paper) defines (N - 1)x(M - 1) squares
	indices.resize((squareSizeX + 1) * (squareSizeZ + 1) * 4, 0);
	squares.resize(numSquares, Square());

	for (int x = 0; x < numSquaresX; x++) {
		for (int z = 0; z < numSquaresZ; z++) {
			// note: insert the squares in row-major order
			// (meaning array index equal to square number)
			int i = z * numSquaresX + x;
			Square& q = squares[i];

			q.tlp = vec3f(x * squareSizeX, 0.0f, z * squareSizeZ);
			q.trp = q.tlp + vec3f(squareSizeX, 0.0f, 0.0f);
			q.brp = q.trp + vec3f(0.0f, 0.0f, squareSizeZ);
			q.blp = q.brp - vec3f(squareSizeX, 0.0f, 0.0f);
			q.mid = vec3f((q.tlp.x + q.trp.x) * 0.5f, 0.0f, (q.tlp.z + q.blp.z) * 0.5f);
			q.idx = i;
		}
	}

	LOG << "[CSMFRenderer::BuildQuadArray]\n";
	LOG << "\tnumber of squares: " << squares.size() << "\n";
	LOG << "\tnumSquaresX, numSquaresZ: " << numSquaresX << ", " << numSquaresZ << "\n";
	LOG << "\tviewRadius, viewRadiusSq: " << viewRadius << ", " << viewRadiusSq << "\n";
}

void CSMFRenderer::GetPotentiallyVisibleSquares(const vec3f& p0, const vec3f& p1, int* minSquareIdx, int* maxSquareIdx) {
	// p0 and p1 are world-space coordinates
	int minx = int((p0.x < p1.x)? p0.x: p1.x) / squareSizeX;
	int minz = int((p0.z < p1.z)? p0.z: p1.z) / squareSizeZ;
	int maxx = int((p0.x > p1.x)? p0.x: p1.x) / squareSizeX;
	int maxz = int((p0.z > p1.z)? p0.z: p1.z) / squareSizeZ;

	if (minx < 0) { minx = 0; }
	if (minz < 0) { minz = 0; }
	if (maxx >= numSquaresX) { maxx = numSquaresX - 1; }
	if (maxz >= numSquaresZ) { maxz = numSquaresZ - 1; }

	// we want the indices of the top-left and the bottom-right
	// square in the square of potentially visible squares (they
	// are indexed in ascending order)
	*minSquareIdx = (minz * numSquaresX + minx);
	*maxSquareIdx = (maxz * numSquaresX + maxx);
}



inline static float GetHeight(const float* hm, float x, float z) {
	// assert(readMap != NULL);
	const int hx = int(x / readMap->SQUARE_SIZE);
	const int hz = int(z / readMap->SQUARE_SIZE);
	const int hw = (readMap->mapx + 1);
	// const int hh = (readMap->mapy + 1);
	const int id = (hz * hw) + hx;

	// assert(id >= 0 && id < (hw * hh));
	return hm[id];
}

// reads from facenormals which are not per-vertex
// (they do not have one extra index on the sides)
/*
inline static const vec3f& GetFaceNormal(const vec3f* nm, float x, float z) {
	// note: unorthodox, default SMF renderer uses shading texture for this
	const int nx = int(x / readMap->SQUARE_SIZE);
	const int nz = int(z / readMap->SQUARE_SIZE);
	const int nw = readMap->mapx;
	// const int nh = readMap->mapy;
	const int id = ((nz * nw) + nx) << 1;
	return nm[id];
}
*/

inline static const vec3f& GetVertexNormal(const float* /*hm*/, const RSquare& q, const vec3f& /*tl*/, int /*maxx*/, int /*maxz*/, int x, int /*xlod*/, int z, int /*zlod*/) {
	/*
	// calculate the normal for (x, y) online
	// using its neighbors at our current LOD
	vec3f tr, bl;
	vec3f e0, e1;

	// [SmDi] hw: 513, hh: 513, hw * hh: 263169
	// [SmDi] maximum values (wxx=4096, wzz=4096)
	const int wxx = int(q.tlp.x) + x;
	const int wzz = int(q.tlp.z) + z;
	const bool ismaxx = (wxx == maxx + 1);
	const bool ismaxz = (wzz == maxz + 1);

	// right-most big square, right-most vertex column (the last valid
	// heightmap col index on a map 512 sq-units wide is 512, not 511)
	// maxx is the maximum world-space x-coordinate (inclusive)
	if (ismaxx) {
		tr = vec3f(wxx - xlod, 0.0f, wzz); tr.y = GetHeight(hm, tr.x, tr.z);
		e0 = tr - tl;
	} else {
		// assert((wxx + xlod <= maxx + 1));
		tr = vec3f(wxx + xlod, 0.0f, wzz); tr.y = GetHeight(hm, tr.x, tr.z);
		e0 = tl - tr;
	}

	// bottom-most big square, bottom-most vertex row (the last valid
	// heightmap row index on a map 512 sq-units high is 512, not 511)
	// maxz is the maximum world-space z-coordinate (inclusive)
	if (ismaxz) {
		bl = vec3f(wxx, 0.0f, wzz - zlod); bl.y = GetHeight(hm, bl.x, bl.z);
		e1 = bl - tl;
	} else {
		// assert((wzz + zlod <= maxz + 1));
		bl = vec3f(wxx, 0.0f, wzz + zlod); bl.y = GetHeight(hm, bl.x, bl.z);
		e1 = tl - bl;
	}

	return (e1.cross(e0).inorm());
	*/

	// use the precomputed highest-LOD normal
	const int hx = int((q.tlp.x + x) / readMap->SQUARE_SIZE);
	const int hz = int((q.tlp.z + z) / readMap->SQUARE_SIZE);
	return readMap->vertexNormals[hz * (readMap->mapx + 1) + hx];
}



inline static int GetXLOD(float d, int /*viewRadius*/, int squareSizeX, const std::vector<int>& /*lodDists*/) {
	/*
	const int xshift = int((d / viewRadius) * lodDists.size());
	const int zshift = int((d / viewRadius) * lodDists.size());
	const int xlod = std::min(squareSizeX, 1 << (xshift << 1));
	const int zlod = std::min(squareSizeZ, 1 << (zshift << 1));
	*/
	if (d < (squareSizeX << 0)) { return 0; }
	if (d < (squareSizeX << 1)) { return 1; }
	if (d < (squareSizeX << 2)) { return 2; }
	if (d < (squareSizeX << 3)) { return 3; }
	if (d < (squareSizeX << 4)) { return 4; }
	if (d < (squareSizeX << 5)) { return 4; }
	if (d < (squareSizeX << 6)) { return 5; }
	if (d < (squareSizeX << 7)) { return 5; }

	return 6;
}

inline static int GetZLOD(float d, int /*viewRadius*/, int squareSizeZ, const std::vector<int>& /*lodDists*/) {
	if (d < (squareSizeZ << 0)) { return 0; }
	if (d < (squareSizeZ << 1)) { return 1; }
	if (d < (squareSizeZ << 2)) { return 2; }
	if (d < (squareSizeZ << 3)) { return 3; }
	if (d < (squareSizeZ << 4)) { return 4; }
	if (d < (squareSizeZ << 5)) { return 4; }
	if (d < (squareSizeZ << 6)) { return 5; }
	if (d < (squareSizeZ << 7)) { return 5; }

	return 6;
}



inline bool CSMFRenderer::SquareRowVisible(int squareIdx, const CReadMap* rm, const Camera* cam) {
	const int rown = squareIdx / numSquaresX;
	const int minx = 0;
	const int maxx = rm->mapx << 3;
	const int minz = rown * squareSizeZ;
	const int maxz = minz + squareSizeZ;
	const float miny = readMap->minheight;
	const float maxy = readMap->maxheight;

	const vec3f mins(minx, miny, minz);
	const vec3f maxs(maxx, maxy, maxz);

	return (cam->InView(mins, maxs));
}

inline bool CSMFRenderer::SquareColVisible(int squareIdx, const CReadMap* rm, const Camera* cam) {
	const int coln = squareIdx % numSquaresX;
	const int minx = coln * squareSizeX;
	const int maxx = minx + squareSizeX;
	const int minz = 0;
	const int maxz = rm->mapy << 3;
	const float miny = readMap->minheight;
	const float maxy = readMap->maxheight;

	const vec3f mins(minx, miny, minz);
	const vec3f maxs(maxx, maxy, maxz);

	return (cam->InView(mins, maxs));
}



void CSMFRenderer::DrawSquare(RSquare& q, const vec3f& v, const CReadMap* rm, const float* hm) {
	const float d = v.flen2D() - 0.01f;
	const int xshift = GetXLOD(d, viewRadius, squareSizeX, lodDists);
	const int zshift = GetZLOD(d, viewRadius, squareSizeZ, lodDists);
	const int xlod = (1 << xshift);
	const int zlod = (1 << zshift);

	const int xverts = (squareSizeX / xlod) + 1;		// #vertices in the x-direction for this square
	const int zverts = (squareSizeZ / zlod) + 1;		// #vertices in the z-direction for this square
	const int numQuads = (xverts - 1) * (zverts - 1);	// #quads needed to cover square at wanted LOD
	const bool wantList = (xlod == 1 && zlod == 1);		// highest-LOD squares are rendered from dlists

	int vertex = 0;
	int offset = 0;


	const float r = 1.0f - (xshift * 0.1667f), g = 0.333f, b = (zshift * 0.1667f);
	glColor3f(r, g, b);


	if (q.hasDisLst && wantList) {
		// note: for deformable terrain, the list
		// would possibly need to be invalidated
		glCallList(q.disLstID);
	} else {
		if (!q.hasDisLst && wantList) {
			q.disLstID = glGenLists(1);
			glNewList(q.disLstID, GL_COMPILE);
		}

		va.Initialize();

		// note: LESS OR EQUAL, so that when {x, z}lod == squareSize{X, Z}
		// four vertices (two in each direction) are added instead of one
		for (int x = 0; x <= squareSizeX; x += xlod) {
			for (int z = 0; z <= squareSizeZ; z += zlod) {
				vec3f tl(q.tlp.x + x, 0.0f, q.tlp.z + z); tl.y = GetHeight(hm, tl.x, tl.z);
				vec3f nv = GetVertexNormal(hm, q, tl, rm->maxxpos, rm->maxzpos, x, xlod, z, zlod);

				va.AddVertex(tl, nv, NVECf);

				// the bottom- and right-most row and column
				// of vertices do not define any new quads
				// if ((x < squareSizeX && z < squareSizeZ)) {
				if (!(x & squareSizeX) && !(z & squareSizeZ)) {
					indices[offset++] = vertex + 0;          // tl
					indices[offset++] = vertex + zverts;     // tr
					indices[offset++] = vertex + zverts + 1; // br
					indices[offset++] = vertex + 1;          // bl
				}

				vertex++;
			}
		}

		va.Draw(numQuads, indices);

		if (!q.hasDisLst && wantList) {
			q.hasDisLst = true;
			glEndList();
		}
	}

	glColor3f(1.0f, 1.0f, 1.0f);
}

void CSMFRenderer::DrawPotentiallyVisibleSquares(const Camera* cam, int, int) {
	const CReadMap* rm = readMap;
	const float* hm = rm->GetHeightmap();
	// const vec3f* nm = &rm->facenormals[0];

	// each square is drawn clockwise topside-up
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CW);

	// our normals are always unit-length
	glDisable(GL_NORMALIZE);
	glPolygonMode(GL_FRONT, GL_FILL);

	for (int i = 0; i < numSquares; i++) {
		Square& q = squares[i];

		if (!SquareRowVisible(q.idx, rm, cam)) {
			// advance to start of next row
			i += (numSquaresX - (i % numSquaresX));
			continue;
		}

		// note: slightly inefficient, would be better to use y-extremes of <q>
		vec3f minBounds = q.GetMinBounds(); minBounds.y = rm->currMinHeight;
		vec3f maxBounds = q.GetMaxBounds(); maxBounds.y = rm->currMaxHeight;
		const vec3f v = (cam->pos - q.mid);
		const float dSq = v.sqLen3D();
		const bool draw = (dSq < viewRadiusSq && cam->InView(minBounds, maxBounds));

		if (draw) {
			DrawSquare(q, v, rm, hm);
		}
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_NORMALIZE);

	glDisable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
}

void CSMFRenderer::Render(const Camera* cam) {
	int minSquareIdx = 0;
	int maxSquareIdx = 0;

	glPushMatrix();
		// get all the squares in the square determined
		// by pos2D's and tar2D's minimum- and maximum
		// x- and z-coordinates (extends)
		// GetPotentiallyVisibleSquares(pos2D, tar2D, &minSquareIdx, &maxSquareIdx);
		// draw the squares from the potentially visible set
		// using LOD determined by distance of square center
		// to camera
		DrawPotentiallyVisibleSquares(cam, minSquareIdx, maxSquareIdx);
	glPopMatrix();
}






/*
void CSMFRenderer::DrawPotentiallyVisibleSquares() {
			// note: ((2*2) / (4*4)) != (2 / 4), different LOD'ing
			for (int x = 0; x < squareSizeX; x += xlod) {
				for (int z = 0; z < squareSizeZ; z += zlod) {
					vec3f tl(q.tlp.x + x,        0.0f, q.tlp.z + z       ); tl.y = GetHeight(hm, tl.x, tl.z);
					vec3f tr(q.tlp.x + x + xlod, 0.0f, q.tlp.z + z       ); tr.y = GetHeight(hm, tr.x, tr.z);
					vec3f br(q.tlp.x + x + xlod, 0.0f, q.tlp.z + z + zlod); br.y = GetHeight(hm, br.x, br.z);
					vec3f bl(q.tlp.x + x,        0.0f, q.tlp.z + z + zlod); bl.y = GetHeight(hm, bl.x, bl.z);

					//// const vec3f e0 = tr - tl;
					//// const vec3f e1 = bl - tl;
					//// const vec3f nv = e1.cross(e0).inorm();
					//// glNormal3f(nv.x, nv.y, nv.z);

					glVertex3f(tl.x, tl.y, tl.z);
					glVertex3f(tr.x, tr.y, tr.z);
					glVertex3f(br.x, br.y, br.z);
					glVertex3f(bl.x, bl.y, bl.z);
				}
			}
		}
	}
}
*/
