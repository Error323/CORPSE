#ifndef PFFG_MODELREADERS3O_HDR
#define PFFG_MODELREADERS3O_HDR

#include <map>

#include "./ModelReaderBase.hpp"
#include "./FormatS3O.hpp"
#include "../../Math/vec3.hpp"

struct VertexS3O {
	vec3<float> pos;
	vec3<float> normal;
	float textureX;
	float textureY;
};

struct PieceS3O: public PieceBase {
	~PieceS3O() {}
	const vec3<float>& GetVertexPos(const int idx) const {
		return vertices[idx].pos;
	}

	std::vector<VertexS3O> vertices;
	std::vector<unsigned int> vertexDrawOrder;
	int primitiveType;

	std::vector<vec3<float> > sTangents; // == T(angent) dirs
	std::vector<vec3<float> > tTangents; // == B(itangent) dirs
};

struct TriangleS3O {
	unsigned int v0idx;
	unsigned int v1idx;
	unsigned int v2idx;
	vec3<float> sTangent;
	vec3<float> tTangent;
};



typedef std::map<std::string, ModelBase*> ModelMap;

class CModelReaderS3O: public CModelReaderBase {
	public:
		CModelReaderS3O();
		~CModelReaderS3O();
		ModelBase* Load(const std::string& name);

	private:
		PieceS3O* LoadPiece(unsigned char*, int, ModelBase*, unsigned int depth = 0);
		void SetVertexTangents(PieceS3O*);
		void FindMinMax(PieceS3O*);

		PieceBase* ClonePiece(PieceBase*) const;
		ModelBase* CloneModel(const ModelBase*) const;

		ModelMap loadedModels;
};

extern CModelReaderS3O* readerS3O;

#endif
