#include <GL/gl.h>

#include "./ModelDrawerS3O.hpp"
#include "./ModelReaderBase.hpp"
#include "./ModelReaderS3O.hpp"
#include "../Textures/TextureHandlerS3O.hpp"

CModelDrawerS3O* drawerS3O = 0x0;



static inline void DrawNormals(PieceS3O* p) {
	if (p->primitiveType == PRIMTYPE_QUADS) { return; }

	// normals (N's)
	glColor3f(1.0f, 0.0f, 0.0f);
	for (unsigned int i = 0; i < p->vertexCount; i++) {
		const VertexS3O& v = p->vertices[i];
		const vec3f vppos = v.pos + (v.normal * 1.0f);

		glVertex3f(v.pos.x, v.pos.y, v.pos.z);
		glVertex3f(vppos.x, vppos.y, vppos.z);
	}
}

static inline void DrawTangents(PieceS3O* p) {
	if (p->primitiveType == PRIMTYPE_QUADS) { return; }

	// S-tangents (T's)
	glColor3f(0.0f, 1.0f, 0.0f);
	for (unsigned int i = 0; i < p->vertexCount; i++) {
		const VertexS3O& v = p->vertices[i];
		const vec3f vppos = v.pos + (p->sTangents[i] * 1.0f);

		glVertex3f(v.pos.x, v.pos.y, v.pos.z);
		glVertex3f(vppos.x, vppos.y, vppos.z);
	}

	// T-tangents (B's)
	glColor3f(0.0f, 0.0f, 1.0f);
	for (unsigned int i = 0; i < p->vertexCount; i++) {
		const VertexS3O& v = p->vertices[i];
		const vec3f vppos = v.pos + (p->tTangents[i] * 1.0f);

		glVertex3f(v.pos.x, v.pos.y, v.pos.z);
		glVertex3f(vppos.x, vppos.y, vppos.z);
	}
}



void CModelDrawerS3O::DrawPieceArrays(PieceBase* p) {
	if (p->isEmpty)
		return;

	PieceS3O* s3op = static_cast<PieceS3O*>(p);
	VertexS3O* s3ov = static_cast<VertexS3O*>(&s3op->vertices[0]);

	if (!s3op->sTangents.empty()) {
		glClientActiveTexture(GL_TEXTURE5);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, sizeof(vec3f), &s3op->sTangents[0].x);

		glClientActiveTexture(GL_TEXTURE6);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, sizeof(vec3f), &s3op->tTangents[0].x);
	}

	glClientActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, sizeof(VertexS3O), &s3ov->textureX);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(VertexS3O), &s3ov->pos.x);

	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, sizeof(VertexS3O), &s3ov->normal.x);

	switch (s3op->primitiveType) {
		case PRIMTYPE_TRIANGLES:
			glDrawElements(GL_TRIANGLES, s3op->vertexDrawOrder.size(), GL_UNSIGNED_INT, &s3op->vertexDrawOrder[0]);
			break;
		case PRIMTYPE_TRIANGLE_STRIP:
			glDrawElements(GL_TRIANGLE_STRIP, s3op->vertexDrawOrder.size(), GL_UNSIGNED_INT, &s3op->vertexDrawOrder[0]);
			break;
		case PRIMTYPE_QUADS:
			glDrawElements(GL_QUADS, s3op->vertexDrawOrder.size(), GL_UNSIGNED_INT, &s3op->vertexDrawOrder[0]);
			break;
	}

	if (!s3op->sTangents.empty()) {
		glClientActiveTexture(GL_TEXTURE6);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		glClientActiveTexture(GL_TEXTURE5);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	glClientActiveTexture(GL_TEXTURE0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);



	#ifdef PFFG_DEBUG_DRAW
	glPushAttrib(GL_CURRENT_BIT | GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT);
		glDisable(GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);
		glBegin(GL_LINES);
		glLineWidth(5.0f);

		DrawNormals(s3op);
		DrawTangents(s3op);

		glLineWidth(1.0f);
		glEnd();
		glColor3f(1.0f, 1.0f, 1.0f);
	glPopAttrib();
	#endif
}

void CModelDrawerS3O::GenListsRec(PieceBase* pb) {
	pb->displayListID = glGenLists(1);
	glNewList(pb->displayListID, GL_COMPILE);
	DrawPieceArrays(pb);
	glEndList();

	for (std::vector<PieceBase*>::iterator pit = pb->children.begin(); pit != pb->children.end(); pit++) {
		PieceBase* cpb = *pit;
		GenListsRec(cpb);
	}
}

void CModelDrawerS3O::Init(PieceBase* pb) {
	GenListsRec(pb);
}

void CModelDrawerS3O::Draw(const PieceBase* pb) const {
	glTranslatef(pb->offset.x, pb->offset.y, pb->offset.z);
	glCallList(pb->displayListID);

	for (std::vector<PieceBase*>::const_iterator pit = pb->children.begin(); pit != pb->children.end(); pit++) {
		Draw(*pit);
	}

	glTranslatef(-pb->offset.x, -pb->offset.y, -pb->offset.z);
}

void CModelDrawerS3O::Free(const PieceBase* pb) const {
	glDeleteLists(pb->displayListID, 1);

	for (std::vector<PieceBase*>::const_iterator pit = pb->children.begin(); pit != pb->children.end(); pit++) {
		Free(*pit);
	}
}
