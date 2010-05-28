#include <cstring>
#include <cassert>
#include <cmath>

#include "./FormatS3O.hpp"
#include "./ModelReaderS3O.hpp"
#include "./ModelDrawerS3O.hpp"
#include "../Textures/TextureHandlerS3O.hpp"
#include "../../System/ByteOrder.hpp"
#include "../../System/EngineAux.hpp"
#include "../../System/FileHandler.hpp"
#include "../../System/Logger.hpp"

CModelReaderS3O* readerS3O = 0x0;

CModelReaderS3O::CModelReaderS3O() {
	drawerS3O = new CModelDrawerS3O();
	textureHandlerS3O = new CTextureHandlerS3O();
}

CModelReaderS3O::~CModelReaderS3O() {
	for (ModelMap::iterator it = loadedModels.begin(); it != loadedModels.end(); it++) {
		ModelBase* m = it->second;
		// destroy the display list tree
		m->drawer->Free(m->rootPiece);
		delete m;
	}

	delete drawerS3O; drawerS3O = 0x0;
	delete textureHandlerS3O; textureHandlerS3O = 0x0;
}



ModelBase* CModelReaderS3O::Load(const std::string& name) {
	if (loadedModels.find(name) != loadedModels.end()) {
		return CloneModel(loadedModels[name]);
	}

	CFileHandler file(name);

	if (!file.FileExists()) {
		LOG << "[CModelReaderS3O::Load]\n";
		LOG << "\tcould not open S3O \"" << name << "\"\n";
		assert(false);
		return 0x0;
	}

	unsigned char* fileBuf = new unsigned char[file.FileSize()];
	file.Read(fileBuf, file.FileSize());

	// needs to be swap()'ed on BE machines
	RawS3O::RHeader header;
	memcpy(&header, fileBuf, sizeof(header));

	ModelBase* model = new ModelBase();
		model->drawer = drawerS3O;
		model->texturer = textureHandlerS3O;
		model->type = MODELTYPE_S3O;
		model->numObjects = 0;
		model->name = name;
		model->tex1 = (char*) &fileBuf[header.tex1offset];
		model->tex2 = (char*) &fileBuf[header.tex2offset];

	textureHandlerS3O->Load(model);

	LOG << "[CModelReaderS3O::Load]\n";
	LOG << "\t(header) magic string:      \"" << header.magic << "\"\n";
	LOG << "\t(header) root piece offset:  "  << header.rootPieceOffset << "\n";
	LOG << "\t(header) radius:             "  << header.radius << "\n";
	LOG << "\t(header) height:             "  << header.height << "\n";
	LOG << "\t(header) midx:               "  << header.midx << "\n";
	LOG << "\t(header) midy:               "  << header.midy << "\n";
	LOG << "\t(header) midz:               "  << header.midz << "\n";
	LOG << "\t(header) tex. 1 name offset: "  << header.tex1offset << "\n";
	LOG << "\t(header) tex. 2 name offset: "  << header.tex2offset << "\n";
	LOG << "\t(model base) texture 1:     \"" << model->tex1 << "\"\n";
	LOG << "\t(model base) texture 2:     \"" << model->tex2 << "\"\n";

	PieceS3O* piece = LoadPiece(fileBuf, header.rootPieceOffset, model);
	piece->type = MODELTYPE_S3O;

	FindMinMax(piece);

	model->rootPiece = piece;
	model->radius = header.radius;
	model->height = header.height;
	model->relMidPos.x = header.midx;
	model->relMidPos.y = header.midy;
	model->relMidPos.z = header.midz;
	model->relMidPos.y = std::max(1.0f, model->relMidPos.y);

	model->maxx = piece->maxx;
	model->maxy = piece->maxy;
	model->maxz = piece->maxz;

	model->minx = piece->minx;
	model->miny = piece->miny;
	model->minz = piece->minz;

	loadedModels[model->name] = model;
	// create the display list tree
	model->drawer->Init(model->rootPiece);

	delete[] fileBuf;
	return CloneModel(model);
}

PieceS3O* CModelReaderS3O::LoadPiece(unsigned char* buf, int offset, ModelBase* model, unsigned int depth) {
	model->numObjects++;

	PieceS3O* piece = new PieceS3O();
	piece->type = MODELTYPE_S3O;

	// needs to be swap()'ed on BE machines
	RawS3O::RPiece* rawPiece = (RawS3O::RPiece*) &buf[offset];

	piece->offset.x = rawPiece->xoffset;
	piece->offset.y = rawPiece->yoffset;
	piece->offset.z = rawPiece->zoffset;
	piece->primitiveType = rawPiece->primitiveType;
	piece->name = (char*) &buf[rawPiece->nameOffset];

	#ifdef PFFG_DEBUG
	std::string tabs = "";
	for (unsigned int k = 0; k < depth; k++) {
		tabs += "\t";
	}
	LOG << tabs + "[CModelReaderS3O::LoadPiece()" << std::endl;
	LOG << tabs + "\tpiece offset: " << offset << std::endl;
	LOG << tabs + "\t(raw piece) nameOffset:        " << rawPiece->nameOffset << "\n";
	LOG << tabs + "\t(raw piece) numChildren:       " << rawPiece->numChildren << "\n";
	LOG << tabs + "\t(raw piece) numVertices:       " << rawPiece->numVertices << "\n";
	LOG << tabs + "\t(raw piece) xoffset:           " << rawPiece->xoffset << "\n";
	LOG << tabs + "\t(raw piece) yoffset:           " << rawPiece->yoffset << "\n";
	LOG << tabs + "\t(raw piece) zoffset:           " << rawPiece->zoffset << "\n";
	LOG << tabs + "\t(raw piece) childTableOffset:  " << rawPiece->childTableOffset << "\n";
	LOG << tabs + "\t(raw piece) vertexTableOffset: " << rawPiece->vertexTableOffset << "\n";
	LOG << tabs + "\t(raw piece) vertexTableSize:   " << rawPiece->vertexTableSize << "\n";
	LOG << tabs + "\t(S3O piece) primitive type:    " << piece->primitiveType << "\n";
	LOG << tabs + "\t(S3O piece) name:              " << piece->name << "\n";
	#endif

	// retrieve each vertex
	int vertexOffset = rawPiece->vertexOffset;

	for (int a = 0; a < rawPiece->numVertices; ++a) {
		// ((RawS3O::RVertex*) &buf[vertexOffset])->swap();
		VertexS3O v = *(VertexS3O*) &buf[vertexOffset];

		piece->vertices.push_back(v);
		vertexOffset += sizeof(RawS3O::RVertex);
	}


	// retrieve the draw order for the vertices
	int vertexTableOffset = rawPiece->vertexTableOffset;

	for (int a = 0; a < rawPiece->vertexTableSize; ++a) {
		// needs to be swabdword()'ed on BE machines
		int vertexDrawIdx = *(int*) &buf[vertexTableOffset];

		piece->vertexDrawOrder.push_back(vertexDrawIdx);
		vertexTableOffset += sizeof(int);

		// -1 == 0xFFFFFFFF when converted to unsigned
		if (vertexDrawIdx == -1 && a != rawPiece->vertexTableSize - 1) {
			// for triangle strips
			piece->vertexDrawOrder.push_back(vertexDrawIdx);

			vertexDrawIdx = *(int*) &buf[vertexTableOffset];
			piece->vertexDrawOrder.push_back(vertexDrawIdx);
		}
	}


	piece->isEmpty = piece->vertexDrawOrder.empty(); 
	piece->vertexCount = piece->vertices.size();

	// calculate the S- and T-tangents
	SetVertexTangents(piece);


	int childTableOffset = rawPiece->childTableOffset;

	for (int a = 0; a < rawPiece->numChildren; ++a) {
		// needs to be swabdword()'ed on BE machines
		int childOffset = *(int*) &buf[childTableOffset];

		PieceS3O* lp = LoadPiece(buf, childOffset, model, depth + 1);
		piece->children.push_back(lp);

		childTableOffset += sizeof(int);
	}

	return piece;
}


void CModelReaderS3O::SetVertexTangents(PieceS3O* p) {
	if (p->isEmpty || p->primitiveType == PRIMTYPE_QUADS) {
		return;
	}

	// LOG << "[CModelReaderS3O::SetVertexTangents]\n";
	// LOG << "\tpiece: " << p << ", primType: " << p->primitiveType << "\n";
	// filter out duplicate vertices somehow
	// no need to store (A, B) *and* (B, A)
	/*
	for (unsigned int i = 0; i < p->vertexCount; i++) {
		const VertexS3O* v0 = &p->vertices[i];

		for (unsigned int j = 0; j < p->vertexCount; j++) {
			if (j != i) {
				const VertexS3O* v1 = &p->vertices[j];

				const float dx = fabsf(v0->pos.x - v1->pos.x);
				const float dy = fabsf(v0->pos.y - v1->pos.y);
				const float dz = fabsf(v0->pos.z - v1->pos.z);

				if (dx < 0.001f && dy < 0.001f && dz < 0.001f) {
					LOG << "\tvertex j=" << j << " is ";
					LOG << "duplicate of i=" << i << "\n";
				}
			}
		}
	}
	*/

	p->sTangents.resize(p->vertexCount, ZVECf);
	p->tTangents.resize(p->vertexCount, ZVECf);

	std::vector<TriangleS3O> triangles;
	std::vector<std::vector<unsigned int> > verts2tris(p->vertexCount);

	unsigned int stride = 0;

	switch (p->primitiveType) {
		case PRIMTYPE_TRIANGLES: {
			stride = 3;
		} break;
		case PRIMTYPE_TRIANGLE_STRIP: {
			stride = 1;
		} break;
	}

	// for triangle strips, the piece vertex _indices_ are defined
	// by the draw order of the vertices numbered <v, v + 1, v + 2>
	// for v in [0, n - 2]
	const unsigned int vrtMaxNr = (stride == 1)?
		p->vertexDrawOrder.size() - 2:
		p->vertexDrawOrder.size();

	// set the triangle-level S- and T-tangents
	for (unsigned int vrtNr = 0; vrtNr < vrtMaxNr; vrtNr += stride) {
		bool flipWinding = false;

		if (p->primitiveType == PRIMTYPE_TRIANGLE_STRIP) {
			flipWinding = ((vrtNr & 1) == 1); 
		}

		const int v0idx = p->vertexDrawOrder[vrtNr                      ];
		const int v1idx = p->vertexDrawOrder[vrtNr + (flipWinding? 2: 1)];
		const int v2idx = p->vertexDrawOrder[vrtNr + (flipWinding? 1: 2)];

		if (v1idx == -1 || v2idx == -1) {
			// not a valid triangle, skip
			// to start of next tri-strip
			vrtNr += 3; continue;
		}

		const VertexS3O* v0 = &p->vertices[v0idx];
		const VertexS3O* v1 = &p->vertices[v1idx];
		const VertexS3O* v2 = &p->vertices[v2idx];

		const vec3f v1v0 = v1->pos - v0->pos;
		const vec3f v2v0 = v2->pos - v0->pos;

		const float sd1 = v1->textureX - v0->textureX; // u1u0
		const float sd2 = v2->textureX - v0->textureX; // u2u0
		const float td1 = v1->textureY - v0->textureY; // v1v0
		const float td2 = v2->textureY - v0->textureY; // v2v0

		// if d is 0, texcoors are degenerate
		const float d = (sd1 * td2) - (sd2 * td1);
		const bool b = (d > -0.001f && d < 0.001f);
		const float r = b? 1.0f: 1.0f / d;

		// sDir and tDir are orthogonal to the _triangle_ surface
		// normal, not necessarily to the smoothed vertex normals
		// _or_ to each other (but likely they will be very close
		// to orthogonal)
		// note: the matrix (T=sDir, B=tDir, N) transforms vectors
		// in tangent space to object space however, while we want
		// the inverse for our shader... this is easy IF sDir and
		// tDir are orthogonal (M^-1 = M^T), which we assume here
		const vec3f sDir = (v1v0 * -td2 + v2v0 * td1) * r;
		const vec3f tDir = (v1v0 * -sd2 + v2v0 * sd1) * r;

		// equal sDir, always inverted tDir (wrt. above expressions)
		// const vec3f sDir((td2 * v1v0.x - td1 * v2v0.x) * r,  (td2 * v1v0.y - td1 * v2v0.y) * r,  (td2 * v1v0.z - td1 * v2v0.z) * r);
		// const vec3f tDir((sd1 * v2v0.x - sd2 * v1v0.x) * r,  (sd1 * v2v0.y - sd2 * v1v0.y) * r,  (sd1 * v2v0.z - sd2 * v1v0.z) * r);

		TriangleS3O tri;
			tri.v0idx = v0idx;
			tri.v1idx = v1idx;
			tri.v2idx = v2idx;
			tri.sTangent = sDir;
			tri.tTangent = tDir;
		triangles.push_back(tri);

		// save the triangle index
		verts2tris[v0idx].push_back(triangles.size() - 1);
		verts2tris[v1idx].push_back(triangles.size() - 1);
		verts2tris[v2idx].push_back(triangles.size() - 1);
	}

	// set the per-vertex tangents (for each vertex, this
	// is the average of the tangents of all the triangles
	// used by it)
	for (unsigned int vrtNr = 0; vrtNr < p->vertexCount; vrtNr++) {
		for (unsigned int triNr = 0; triNr < verts2tris[vrtNr].size(); triNr++) {
			const unsigned int triIdx = verts2tris[vrtNr][triNr];
			const TriangleS3O& tri    = triangles[triIdx];

			p->sTangents[vrtNr] += tri.sTangent;
			p->tTangents[vrtNr] += tri.tTangent;
		}

		vec3f& s = p->sTangents[vrtNr];
		vec3f& t = p->tTangents[vrtNr];
		vec3f& n = p->vertices[vrtNr].normal;
		int h = 1; // handedness

		if (isnan(n.x) || isnan(n.y) || isnan(n.z)) {
			n = vec3f(0.0f, 1.0f, 0.0f);
		}

		// note: we create smoothed S- and T-tangents,
		// but these possibly do not form an orthogonal
		// basis so we must? perform Gram-Schmidt here
		// s = (s - (n * (n.dot3D(s)))).norm();
		// t = (t - (n * (n.dot3D(t))) - (s * s.dot3D(t))).norm();
		// h = ((s.cross(t)).dot3D(n) >= 0.0f)? 1: -1; same as
		// h = ((n.cross(s)).dot3D(t) >= 0.0f)? 1: -1;

		s = (s - (n * s.dot3D(n))).norm();
		h = ((s.cross(t)).dot3D(n) >= 0.0f)? 1: -1;
		t = (s.cross(n)).norm() * h;
	}
}


void CModelReaderS3O::FindMinMax(PieceS3O* p) {
	std::vector<PieceBase*>::iterator si;

	// pieces with no children terminate the recursion
	for (si = p->children.begin(); si != p->children.end(); ++si) {
		FindMinMax(static_cast<PieceS3O*>(*si));
	}

	float maxx = -1000.0f, maxy = -1000.0f, maxz = -1000.0f;
	float minx = 10000.0f, miny = 10000.0f, minz = 10000.0f;

	std::vector<VertexS3O>::iterator vi;

	for (vi = p->vertices.begin(); vi != p->vertices.end(); ++vi) {
		maxx = std::max(maxx, vi->pos.x);
		maxy = std::max(maxy, vi->pos.y);
		maxz = std::max(maxz, vi->pos.z);

		minx = std::min(minx, vi->pos.x);
		miny = std::min(miny, vi->pos.y);
		minz = std::min(minz, vi->pos.z);
	}

	for (si = p->children.begin(); si != p->children.end(); ++si) {
		maxx = std::max(maxx, (*si)->offset.x + (*si)->maxx);
		maxy = std::max(maxy, (*si)->offset.y + (*si)->maxy);
		maxz = std::max(maxz, (*si)->offset.z + (*si)->maxz);

		minx = std::min(minx, (*si)->offset.x + (*si)->minx);
		miny = std::min(miny, (*si)->offset.y + (*si)->miny);
		minz = std::min(minz, (*si)->offset.z + (*si)->minz);
	}

	p->maxx = maxx;
	p->maxy = maxy;
	p->maxz = maxz;
	p->minx = minx;
	p->miny = miny;
	p->minz = minz;
}



PieceBase* CModelReaderS3O::ClonePiece(PieceBase* src) const {
	PieceS3O* dst = new PieceS3O();
	assert(dst != 0x0);
	dst->type = src->type;
	dst->name = src->name;
	dst->vertexCount = src->vertexCount;
	dst->displayListID = src->displayListID; // could become an issue?
	dst->offset = src->offset;
	dst->isEmpty = src->isEmpty;

	dst->maxx = src->maxx;
	dst->maxy = src->maxy;
	dst->maxz = src->maxz;

	dst->minx = src->minx;
	dst->miny = src->miny;
	dst->minz = src->minz;

	PieceS3O* dsrc = dynamic_cast<PieceS3O*>(src);
	assert(dsrc != 0x0);

	for (unsigned int i = 0; i < dsrc->vertices.size(); i++) {
		dst->vertices.push_back(dsrc->vertices[i]);

		if (i < dsrc->sTangents.size()) {
			// NOTE: sometimes false?
			dst->sTangents.push_back(dsrc->sTangents[i]);
			dst->tTangents.push_back(dsrc->tTangents[i]);
		}
	}
	for (unsigned int i = 0; i < dsrc->vertexDrawOrder.size(); i++) {
		dst->vertexDrawOrder.push_back(dsrc->vertexDrawOrder[i]);
	}

	for (unsigned int i = 0; i < src->children.size(); i++) {
		dst->children.push_back(ClonePiece(src->children[i]));
	}

	return dst;
}

ModelBase* CModelReaderS3O::CloneModel(const ModelBase* src) const {
	ModelBase* dst = new ModelBase();
	assert(src != 0x0);
	assert(dst != 0x0);

	dst->drawer = src->drawer;
	dst->texturer = src->texturer;

	dst->type = src->type;
	dst->numObjects = src->numObjects;
	dst->name = src->name;

	dst->textureType = src->textureType;
	dst->tex1 = src->tex1;
	dst->tex2 = src->tex2;

	dst->radius = src->radius;
	dst->height = src->height;
	dst->relMidPos = src->relMidPos;

	dst->maxx = src->maxx;
	dst->maxy = src->maxy;
	dst->maxz = src->maxz;

	dst->minx = src->minx;
	dst->miny = src->miny;
	dst->minz = src->minz;

	dst->rootPiece = ClonePiece(src->rootPiece);
	return dst;
}
