#ifndef PFFG_MODELREADER3DO_HDR
#define PFFG_MODELREADER3DO_HDR

#include <set>

#include "./Format3DO.hpp"
#include "./ModelReaderBase.hpp"
#include "../../Math/vec3.hpp"

struct Vertex3DO {
	vec3f pos;
	vec3f normal;
	std::vector<int> prims;
};

struct Primitive3DO {
	std::vector<int> vertices;
	std::vector<vec3f> normals;		// per-vertex normals
	vec3f normal;
	int numVertex;
	// C3DOTextureHandler::UnitTexture* texture;
};


struct Piece3DO: public PieceBase {
	const vec3f& GetVertexPos(const int idx) const {
		return vertices[idx].pos;
	}

	std::vector<Vertex3DO> vertices;
	std::vector<Primitive3DO> prims;
	float radius;
	vec3f relMidPos;
};

class CModelReader3DO: public CModelReaderBase {
	typedef std::vector<vec3f> VertexVector;

	public:
		CModelReader3DO();
		~CModelReader3DO();
		ModelBase* Load(const std::string& name);

	private:
		void FindCenter(Piece3DO* object);
		float FindRadius(Piece3DO* object, vec3f offset);
		float FindHeight(Piece3DO* object, vec3f offset);
		void CalcNormals(Piece3DO* object);

		void GetPrimitives(Piece3DO* obj, int pos, int num, VertexVector* vv, int excludePrim);
		void GetVertices(Raw3DO::R3DObject* o, Piece3DO* object);
		std::string GetText(int pos);
		bool ReadChild(int pos, Piece3DO* root, int* numObj);

		std::set<std::string> teamtex;

		int curOffset;
		unsigned char* fileBuf;
		void SimStreamRead(void* buf, int length);
};

extern CModelReader3DO* reader3DO;

#endif
