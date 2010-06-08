#ifndef PFFG_MODELREADERS3O_HDR
#define PFFG_MODELREADERS3O_HDR

#include <map>

#include "./ModelReaderBase.hpp"
#include "./FormatS3O.hpp"
#include "../../Math/vec3fwd.hpp"
#include "../../Math/vec3.hpp"

struct VertexS3O {
	vec3f pos;
	vec3f normal;
	float textureX;
	float textureY;
};

struct PieceS3O: public PieceBase {
	PieceS3O() { parent = NULL; }
	~PieceS3O() {
		vertices.clear();
		vertexDrawOrder.clear();

		sTangents.clear();
		tTangents.clear();
	}

	void DrawList() const {}
	void SetMinMaxExtends();
	void SetVertexTangents();
	const vec3f& GetVertexPos(int idx) const { return vertices[idx].pos; }
	void Shatter(float, int, int, const vec3f&, const vec3f&) const {}

	std::vector<VertexS3O> vertices;
	std::vector<unsigned int> vertexDrawOrder;
	int primitiveType;

	std::vector<vec3f> sTangents; // == T(angent) dirs
	std::vector<vec3f> tTangents; // == B(itangent) dirs
};

struct TriangleS3O {
	unsigned int v0idx;
	unsigned int v1idx;
	unsigned int v2idx;
	vec3f sTangent;
	vec3f tTangent;
};



typedef std::map<std::string, ModelBase*> ModelMap;

class CModelReaderS3O: public CModelReaderBase {
	public:
		CModelReaderS3O();
		~CModelReaderS3O();
		ModelBase* Load(const std::string& name);

	private:
		PieceS3O* LoadPiece(ModelBase*, PieceS3O*, unsigned char*, int, unsigned int);

		PieceBase* ClonePiece(PieceBase*) const;
		ModelBase* CloneModel(const ModelBase*) const;

		ModelMap loadedModels;
};

extern CModelReaderS3O* readerS3O;

#endif
