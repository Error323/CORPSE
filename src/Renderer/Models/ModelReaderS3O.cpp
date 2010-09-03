#include <cstring>
#include <cmath>

#include "./FormatS3O.hpp"
#include "./ModelReaderS3O.hpp"
#include "./ModelDrawerS3O.hpp"
#include "../Textures/TextureHandlerS3O.hpp"
#include "../../System/ByteOrder.hpp"
#include "../../System/EngineAux.hpp"
#include "../../System/FileHandler.hpp"
#include "../../System/Logger.hpp"
#include "../../System/Debugger.hpp"

#define SWAP(x) (x)

static const vec3f DEF_MIN_SIZE( 10000.0f,  10000.0f,  10000.0f);
static const vec3f DEF_MAX_SIZE(-10000.0f, -10000.0f, -10000.0f);

CModelReaderS3O* readerS3O = NULL;

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

	delete drawerS3O; drawerS3O = NULL;
	delete textureHandlerS3O; textureHandlerS3O = NULL;
}



ModelBase* CModelReaderS3O::Load(const std::string& name) {
	if (loadedModels.find(name) != loadedModels.end()) {
		return CloneModel(loadedModels[name]);
	}

	CFileHandler file(name);

	if (!file.FileExists()) {
		LOG << "[CModelReaderS3O::Load]\n";
		LOG << "\tcould not open S3O \"" << name << "\"\n";
		PFFG_ASSERT(false);
		return NULL;
	}

	unsigned char* fileBuf = new unsigned char[file.FileSize()];
	file.Read(fileBuf, file.FileSize());

	RawS3O::RHeader header;
	memcpy(&header, fileBuf, sizeof(header));

	ModelBase* model = new ModelBase();
		model->name = name;
		model->type = MODELTYPE_S3O;
		model->numObjects = 0;
		model->tex1 = (char*) &fileBuf[header.tex1offset];
		model->tex2 = (char*) &fileBuf[header.tex2offset];
		model->mins = DEF_MIN_SIZE;
		model->maxs = DEF_MAX_SIZE;
		model->drawer = drawerS3O;
		model->texturer = textureHandlerS3O;

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

	PieceS3O* rootPiece = LoadPiece(model, NULL, fileBuf, header.rootPieceOffset, 0);

	model->rootPiece = rootPiece;
	model->radius = header.radius;
	model->height = header.height;
	model->relMidPos = vec3f(header.midx, header.midy, header.midz);
	model->relMidPos.y = std::max(model->relMidPos.y, 1.0f); // ?
	model->radius =
		(((model->maxs.x - model->mins.x) * 0.5f) * ((model->maxs.x - model->mins.x) * 0.5f)) +
		(((model->maxs.y - model->mins.y) * 0.5f) * ((model->maxs.y - model->mins.y) * 0.5f)) +
		(((model->maxs.z - model->mins.z) * 0.5f) * ((model->maxs.z - model->mins.z) * 0.5f));
	model->radius = sqrtf(model->radius);

	loadedModels[model->name] = model;
	// create the display list tree
	model->drawer->Init(model->rootPiece);

	delete[] fileBuf;
	return (CloneModel(model));
}

PieceS3O* CModelReaderS3O::LoadPiece(ModelBase* model, PieceS3O* parent, unsigned char* buf, int offset, unsigned int depth) {
	model->numObjects++;

	RawS3O::RPiece* rawPiece = (RawS3O::RPiece*) &buf[offset];

	PieceS3O* piece = new PieceS3O();
		piece->type = MODELTYPE_S3O;
		piece->mins = DEF_MIN_SIZE;
		piece->maxs = DEF_MAX_SIZE;
		piece->offset.x = rawPiece->xoffset;
		piece->offset.y = rawPiece->yoffset;
		piece->offset.z = rawPiece->zoffset;
		piece->primitiveType = rawPiece->primitiveType;
		piece->name = (char*) &buf[rawPiece->nameOffset];
		piece->parent = parent;

	#ifdef DEBUG
	std::string tabs = "";
	for (unsigned int k = 0; k < depth; k++) {
		tabs += "\t";
	}
	LOG << tabs + "[CModelReaderS3O::LoadPiece()\n";
	LOG << tabs + "\tpiece offset:                  " << offset << "\n";
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
		VertexS3O v = *(VertexS3O*) &buf[vertexOffset];
			v.normal.norm3D();
		piece->vertices.push_back(v);

		vertexOffset += sizeof(VertexS3O);
	}


	// retrieve the draw order for the vertices
	int vertexTableOffset = rawPiece->vertexTableOffset;

	for (int a = 0; a < rawPiece->vertexTableSize; ++a) {
		int vertexDrawIdx = SWAP(*(int*) &buf[vertexTableOffset]);

		piece->vertexDrawOrder.push_back(vertexDrawIdx);
		vertexTableOffset += sizeof(int);

		// -1 == 0xFFFFFFFF (U)
		if (vertexDrawIdx == -1 && a != rawPiece->vertexTableSize - 1) {
			// for triangle strips
			piece->vertexDrawOrder.push_back(vertexDrawIdx);

			vertexDrawIdx = SWAP(*(int*) &buf[vertexTableOffset]);
			piece->vertexDrawOrder.push_back(vertexDrawIdx);
		}
	}

	piece->isEmpty = piece->vertexDrawOrder.empty();
	piece->vertexCount = piece->vertices.size();
	piece->goffset = piece->offset + ((parent != NULL)? parent->goffset: NVECf);

	piece->SetVertexTangents();
	piece->SetMinMaxExtends();

	model->mins.x = std::min(piece->mins.x, model->mins.x);
	model->mins.y = std::min(piece->mins.y, model->mins.y);
	model->mins.z = std::min(piece->mins.z, model->mins.z);
	model->maxs.x = std::max(piece->maxs.x, model->maxs.x);
	model->maxs.y = std::max(piece->maxs.y, model->maxs.y);
	model->maxs.z = std::max(piece->maxs.z, model->maxs.z);

	int childTableOffset = rawPiece->childTableOffset;

	for (int a = 0; a < rawPiece->numChildren; ++a) {
		int childOffset = SWAP(*(int*) &buf[childTableOffset]);

		PieceS3O* childPiece = LoadPiece(model, piece, buf, childOffset, depth + 1);
		piece->children.push_back(childPiece);

		childTableOffset += sizeof(int);
	}

	return piece;
}



PieceBase* CModelReaderS3O::ClonePiece(PieceBase* src) const {
	PieceS3O* dst = new PieceS3O();
	PFFG_ASSERT(dst != NULL);
	dst->type = src->type;
	dst->name = src->name;
	dst->vertexCount = src->vertexCount;
	dst->displayListID = src->displayListID; // could become an issue?
	dst->offset = src->offset;
	dst->isEmpty = src->isEmpty;

	dst->maxs = src->maxs;
	dst->mins = src->mins;

	PieceS3O* dsrc = dynamic_cast<PieceS3O*>(src);
	PFFG_ASSERT(dsrc != NULL);

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
	PFFG_ASSERT(src != NULL);
	PFFG_ASSERT(dst != NULL);

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

	dst->maxs = src->maxs;
	dst->mins = src->mins;

	dst->rootPiece = ClonePiece(src->rootPiece);
	return dst;
}



void PieceS3O::SetMinMaxExtends() {
	for (std::vector<VertexS3O>::const_iterator vi = vertices.begin(); vi != vertices.end(); ++vi) {
		mins.x = std::min(mins.x, (goffset.x + vi->pos.x));
		mins.y = std::min(mins.y, (goffset.y + vi->pos.y));
		mins.z = std::min(mins.z, (goffset.z + vi->pos.z));
		maxs.x = std::max(maxs.x, (goffset.x + vi->pos.x));
		maxs.y = std::max(maxs.y, (goffset.y + vi->pos.y));
		maxs.z = std::max(maxs.z, (goffset.z + vi->pos.z));
	}
}

void PieceS3O::SetVertexTangents() {
	if (isEmpty || primitiveType == PRIMTYPE_QUADS) {
		return;
	}

	sTangents.resize(vertexCount, NVECf);
	tTangents.resize(vertexCount, NVECf);

	unsigned stride = 0;

	switch (primitiveType) {
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
	const unsigned vrtMaxNr = (stride == 1)?
		vertexDrawOrder.size() - 2:
		vertexDrawOrder.size();

	// set the triangle-level S- and T-tangents
	for (unsigned vrtNr = 0; vrtNr < vrtMaxNr; vrtNr += stride) {
		bool flipWinding = false;

		if (primitiveType == PRIMTYPE_TRIANGLE_STRIP) {
			flipWinding = ((vrtNr & 1) == 1);
		}

		const int v0idx = vertexDrawOrder[vrtNr                      ];
		const int v1idx = vertexDrawOrder[vrtNr + (flipWinding? 2: 1)];
		const int v2idx = vertexDrawOrder[vrtNr + (flipWinding? 1: 2)];

		if (v1idx == -1 || v2idx == -1) {
			// not a valid triangle, skip
			// to start of next tri-strip
			vrtNr += 3; continue;
		}

		const VertexS3O* vrt0 = &vertices[v0idx];
		const VertexS3O* vrt1 = &vertices[v1idx];
		const VertexS3O* vrt2 = &vertices[v2idx];

		const vec3f& p0 = vrt0->pos;
		const vec3f& p1 = vrt1->pos;
		const vec3f& p2 = vrt2->pos;

		const vec3f tc0(vrt0->textureX, vrt0->textureY, -1.0f);
		const vec3f tc1(vrt1->textureX, vrt1->textureY, -1.0f);
		const vec3f tc2(vrt2->textureX, vrt2->textureY, -1.0f);

		const float x1x0 = p1.x - p0.x, x2x0 = p2.x - p0.x;
		const float y1y0 = p1.y - p0.y, y2y0 = p2.y - p0.y;
		const float z1z0 = p1.z - p0.z, z2z0 = p2.z - p0.z;

		const float s1 = tc1.x - tc0.x, s2 = tc2.x - tc0.x;
		const float t1 = tc1.y - tc0.y, t2 = tc2.y - tc0.y;

		// if d is 0, texcoors are degenerate
		const float d = (s1 * t2 - s2 * t1);
		const bool  b = (d > -0.0001f && d < 0.0001f);
		const float r = b? 1.0f: 1.0f / d;

		// note: not necessarily orthogonal to each other
		// or to vertex normal (only to the triangle plane)
		const vec3f sdir((t2 * x1x0 - t1 * x2x0) * r, (t2 * y1y0 - t1 * y2y0) * r, (t2 * z1z0 - t1 * z2z0) * r);
		const vec3f tdir((s1 * x2x0 - s2 * x1x0) * r, (s1 * y2y0 - s2 * y1y0) * r, (s1 * z2z0 - s2 * z1z0) * r);

		sTangents[v0idx] += sdir;
		sTangents[v1idx] += sdir;
		sTangents[v2idx] += sdir;

		tTangents[v0idx] += tdir;
		tTangents[v1idx] += tdir;
		tTangents[v2idx] += tdir;
	}

	// set the smoothed per-vertex tangents
	for (int vrtIdx = vertices.size() - 1; vrtIdx >= 0; vrtIdx--) {
		vec3f& n = vertices[vrtIdx].normal;
		vec3f& s = sTangents[vrtIdx];
		vec3f& t = tTangents[vrtIdx];
		int h = 1;

		if (isnan(n.x) || isnan(n.y) || isnan(n.z)) {
			n = vec3f(0.0f, 0.0f, 1.0f);
		}
		if (s == NVECf) { s = vec3f(1.0f, 0.0f, 0.0f); }
		if (t == NVECf) { t = vec3f(0.0f, 1.0f, 0.0f); }

		h = ((n.cross(s)).dot3D(t) < 0.0f)? -1: 1;
		s = (s - n * n.dot3D(s));
		s = s.norm3D();
		t = (s.cross(n)) * h;

		// t = (s.cross(n));
		// h = ((s.cross(t)).dot(n) >= 0.0f)? 1: -1;
		// t = t * h;
	}
}
