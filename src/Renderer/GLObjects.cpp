#include <GL/gl.h>
#include "./GLObjects.hpp"

namespace GL {
	void Light::Enable() { enabled = true; glEnable(GL_LIGHT0 + lightIdx); }
	void Light::Disable() { enabled = false; glDisable(GL_LIGHT0 + lightIdx); }



	VertexArray::VertexArray() {
		Initialize();
	}

	void VertexArray::Initialize() {
			vDrawIdx = 0;
			tDrawIdx = 0;
			nDrawIdx = 0;
		}
	void VertexArray::Enlarge() {
		if ((vDrawIdx + 3) > verts.size()) { verts.resize((verts.size() + 3) * 2, 0.0f); }
		if ((nDrawIdx + 3) > norms.size()) { norms.resize((norms.size() + 3) * 2, 0.0f); }
	}

	void VertexArray::AddVertex(float vx, float vy, float vz, float nx, float ny, float nz, float /*tu*/, float /*tv*/) {
		Enlarge();

		// (vDrawIdx / 3) is always equal to vertex (vec3f) count
		// (nDrawIdx / 3) is always equal to normal (vec3f) count
		verts[vDrawIdx++] = vx;
		verts[vDrawIdx++] = vy;
		verts[vDrawIdx++] = vz;
		norms[nDrawIdx++] = nx;
		norms[nDrawIdx++] = ny;
		norms[nDrawIdx++] = nz;
	}
	void VertexArray::AddVertex(const vec3f& v, const vec3f& n, const vec3f& tc) {
		AddVertex(v.x, v.y, v.z, n.x, n.y, n.z, tc.x, tc.y);
	}

	void VertexArray::Draw(int numQuads, const std::vector<int>& indices) {
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);

		glVertexPointer(3, GL_FLOAT, 0, &verts[0]);
		glNormalPointer(/*3,*/ GL_FLOAT, 0, &norms[0]);
		glDrawElements(GL_QUADS, numQuads * 4, GL_UNSIGNED_INT, &indices[0]);
		// cannot use this due to vertex insertion order
		// glDrawArrays(GL_QUADS, 0, numQuads);

		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
	}
		
	vec3f VertexArray::GetVertex(int b) const { return vec3f(verts[b], verts[b + 1], verts[b + 2]); }
	vec3f VertexArray::GetNormal(int b) const { return vec3f(norms[b], norms[b + 1], norms[b + 2]); }




	VertexArrayManager::VertexArrayManager(): va0(0x0), va1(0x0), va(0x0) {
		va0 = new VertexArray();
		va1 = new VertexArray();
	}
	VertexArrayManager::~VertexArrayManager() {
		delete va0; va0 = 0x0;
		delete va1; va1 = 0x0;
	}

	VertexArray* VertexArrayManager::GetVertexArray() {
		if (va == va0) {
			va = va1;
		} else {
			va = va0;
		}
		return va;
	}
}
