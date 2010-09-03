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
	std::string name;
	std::vector<PieceBase*> children;

	PieceBase* parent;

	bool isEmpty;
	unsigned int vertexCount;
	unsigned int displayListID;

	// MODELTYPE_*
	int type;

	vec3f mins;
	vec3f maxs;
	vec3f offset;    // wrt. parent
	vec3f goffset;   // wrt. root

	virtual ~PieceBase() {}
	virtual void DrawList() const = 0;
	virtual int GetVertexCount() const { return vertexCount; }
	virtual int GetNormalCount() const { return 0; }
	virtual int GetTxCoorCount() const { return 0; }
	virtual void SetMinMaxExtends() {}
	virtual void SetVertexTangents() {}
	virtual const vec3f& GetVertexPos(int) const = 0;
	virtual void Shatter(vec3f, int, int, const vec3f&, const vec3f&) const {}
	void DrawStatic() const;
};

// generic base for 3DO and S3O models (piece hierarchies)
// note that there are not actually any Model3DO / ModelS3O
// structures deriving from this, instead <type> serves as
// the runtime identifier
struct ModelBase {
public:
	ModelBase(): shaderProObj(NULL) {}
	~ModelBase() { DelPiece(rootPiece); }

	void DelPiece(PieceBase* p) {
		for (unsigned int i = 0; i < p->children.size(); i++) {
			DelPiece(p->children[i]);
		}

		delete p;
	}

	int type;               //! MODELTYPE_*
	int textureType;        //! FIXME: MAKE S3O ONLY (0 = 3DO, otherwise S3O or OBJ)

	std::string name;
	std::string tex1;
	std::string tex2;

	int numObjects;
	float scale;
	float radius;
	float height;

	vec3f mins;
	vec3f maxs;
	vec3f relMidPos;

	PieceBase* rootPiece;
	CModelDrawerBase* drawer;
	CTextureHandlerBase* texturer;

	void SetShaderProgramObj(const Shader::IProgramObject* obj) { shaderProObj = obj; }
	const Shader::IProgramObject* GetShaderProgramObj() const { return shaderProObj; }

private:
	const Shader::IProgramObject* shaderProObj;

};



struct LocalModel {
public:
	LocalModel(const ModelBase* _m): m(_m) {}
	const ModelBase* GetModelBase() const { return m; }

private:
	const ModelBase* m;
};



// "IModelParser" in Spring
// TODO: also import LocalPiece and LocalModel (3DModel.h)
class CModelReaderBase {
	public:
		virtual ~CModelReaderBase() {}
		virtual ModelBase* Load(const std::string& name) = 0;
};

#endif
