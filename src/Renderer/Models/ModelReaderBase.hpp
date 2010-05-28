#ifndef PFFG_MODELREADERBASE_HDR
#define PFFG_MODELREADERBASE_HDR

#include <iostream>
#include <vector>

#include "../../Math/vec3fwd.hpp"
#include "../../Math/mat44fwd.hpp"
#include "../../Math/vec3.hpp"
#include "../../Math/mat44.hpp"
#include "../Shaders/Shader.hpp"

enum ModelType {MODELTYPE_3DO, MODELTYPE_S3O, MODELTYPE_OTHER};
enum PrimType {PRIMTYPE_TRIANGLES, PRIMTYPE_TRIANGLE_STRIP, PRIMTYPE_QUADS};

class CModelDrawerBase;
class CTextureHandlerBase;



// generic base for 3DO and S3O model pieces
struct PieceBase {
	virtual ~PieceBase() {}
	virtual const vec3f& GetVertexPos(const int idx) const = 0;


	std::string name;
	std::vector<PieceBase*> children;

	unsigned int vertexCount;
	unsigned int displayListID;
	vec3f offset;
	bool isEmpty;
	float maxx, maxy, maxz;
	float minx, miny, minz;

	ModelType type;
};

// generic base for 3DO and S3O models (piece hierarchies)
// note that there are not actually any Model3DO / ModelS3O
// structures deriving from this, instead <type> serves as
// the runtime identifier
struct ModelBase {
	~ModelBase() {
		DelPiece(rootPiece);
	}

	void DelPiece(PieceBase* p) {
		for (unsigned int i = 0; i < p->children.size(); i++) {
			DelPiece(p->children[i]);
		}

		delete p;
	}


	PieceBase* rootPiece;
	int numObjects;
	float radius;
	float height;
	std::string name;
	float maxx, maxy, maxz;
	float minx, miny, minz;
	vec3f relMidPos;

	ModelType type;
	// 0: 3DO, otherwise S3O
	int textureType;

	std::string tex1;
	std::string tex2;

	CModelDrawerBase* drawer;
	CTextureHandlerBase* texturer;
};



struct LocalModel {
public:
	LocalModel(const ModelBase* _m): m(_m), shaderProObj(NULL) {}
	const ModelBase* GetModelBase() const { return m; }

	void SetShaderProgramObj(const Shader::IProgramObject* obj) { shaderProObj = obj; }
	const Shader::IProgramObject* GetShaderProgramObj() const { return shaderProObj; }

private:
	const ModelBase* m;
	const Shader::IProgramObject* shaderProObj;
};



// "IModelParser" in Spring
// TODO: also import LocalPiece and LocalModel (3DModel.h)
class CModelReaderBase {
	public:
		virtual ~CModelReaderBase() {}
		virtual ModelBase* Load(const std::string& name) = 0;
};

#endif
