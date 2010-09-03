#include <cmath>

#include "./SMFGroundDrawer.hpp"
#include "./SMFGroundTextures.hpp"

#include "../../Renderer/Camera.hpp"
#include "../../Renderer/VertexArray.hpp"
#include "../../Renderer/Shaders/ShaderHandler.hpp"
#include "../../Renderer/Env/ShadowHandler.hpp"
#include "../MapInfo.hpp"
#include "../ReadMap.hpp"
#include "./SMFReadMap.hpp"

//// #include "Rendering/ShadowHandler.h"
//// #include "Rendering/GroundDecalHandler.h"
//// #include "Platform/ConfigHandler.h"
#include "../../Math/FastMath.hpp"
#include "../../System/EngineAux.hpp"
#include "../../System/LuaParser.hpp"
#include "../../System/Logger.hpp"



static const float guViewRange = 16384.0f;

CSMFGroundDrawer::CSMFGroundDrawer(CSMFReadMap* smfReadMap):
	bigSquareSize(128),
	numBigTexX(smfReadMap->mapx / bigSquareSize),
	numBigTexY(smfReadMap->mapy / bigSquareSize),
	maxIdx(((smfReadMap->mapx + 1) * (smfReadMap->mapy + 1)) - 1),
	heightDataX(smfReadMap->mapx + 1) {

	mapWidth = (smfReadMap->mapx * smfReadMap->SQUARE_SIZE);
	mapHeight = (smfReadMap->mapy * smfReadMap->SQUARE_SIZE);
	bigTexH = (smfReadMap->mapy * smfReadMap->SQUARE_SIZE) / numBigTexY;

	heightData = smfReadMap->heightmap;
	smfMap = smfReadMap;

	va = new VertexArray();
	textures = new CSMFGroundTextures(smfMap);

	// we need a multiple of 2
	viewRadius = LUA->GetRoot()->GetTblVal("map")->GetFltVal("viewRadius", 1024.0f);
	viewRadius += (viewRadius & 1);



	// load the map shader program
	const std::string shaderDir = LUA->GetRoot()->GetTblVal("general")->GetStrVal("shadersDir", "data/shaders/");
	const std::string vShaderFileSMF = LUA->GetRoot()->GetTblVal("map")->GetStrVal("vShader", "");
	const std::string vShaderPathSMF = shaderDir + vShaderFileSMF;
	const std::string fShaderFileSMF = LUA->GetRoot()->GetTblVal("map")->GetStrVal("fShader", "");
	const std::string fShaderPathSMF = shaderDir + fShaderFileSMF;

	if (!vShaderFileSMF.empty() || !fShaderFileSMF.empty()) {
		if (!vShaderFileSMF.empty()) { LOG << "[CSMFGroundDrawer] loading vert. shader " << vShaderPathSMF << " for map\n"; }
		if (!fShaderFileSMF.empty()) { LOG << "[CSMFGroundDrawer] loading frag. shader " << fShaderPathSMF << " for map\n"; }

		Shader::IProgramObject* pObj = shaderHandler->CreateProgramObject("[SMF]", "SMFShader", vShaderPathSMF, "", fShaderPathSMF, "", false);

		#ifndef CUSTOM_SMF_RENDERER
		shaderProObj = pObj;
		#endif

		LOG << "[CSMFGroundDrawer] shader program object info\n";
		LOG << "\tpObj->GetObjID(): " << pObj->GetObjID() << "\n";
		LOG << "\tpObj->IsValid():  " << pObj->IsValid() << "\n";

		if (pObj->IsValid()) {
			pObj->SetUniformLocation("diffuseMap"); // idx 0
			pObj->SetUniformLocation("overlayMap"); // idx 1
			pObj->SetUniformLocation("shadowMap");  // idx 2
			pObj->SetUniformLocation("shadowMat");  // idx 3
			pObj->SetUniformLocation("texSquareX"); // idx 4
			pObj->SetUniformLocation("texSquareZ"); // idx 5
			pObj->SetUniformLocation("mapSizeX");   // idx 6
			pObj->SetUniformLocation("mapSizeZ");   // idx 7

			pObj->Enable();
			pObj->SetUniform1i(0, 0);          // (idx 0, texunit 0)
			pObj->SetUniform1i(1, 1);          // (idx 1, texunit 1)
			pObj->SetUniform1i(2, 7);          // (idx 2, texunit 7)
			pObj->SetUniform1i(6, mapWidth);
			pObj->SetUniform1i(7, mapHeight);
			pObj->Disable();
		} else {
			PFFG_ASSERT(false);
		}

		LOG << "\n";
		LOG << "\tpObj->GetLog():\n";
		LOG << pObj->GetLog() << "\n";

		const std::vector<const Shader::IShaderObject*>& sObjs = pObj->GetAttachedShaderObjs();

		for (std::vector<const Shader::IShaderObject*>::const_iterator it = sObjs.begin(); it != sObjs.end(); it++) {
			LOG << "\n" << ((*it)->GetLog()) << "\n";
		}
	}
}

CSMFGroundDrawer::~CSMFGroundDrawer(void) {
	delete textures;
	delete va; va = 0x0;

	shaderHandler->ReleaseProgramObjects("[SMF]");
}






inline static void DrawNormal(const vec3f& p0, const vec3f& p1, const Shader::IProgramObject* po) {
	glPushAttrib(GL_ALL_ATTRIB_BITS);
		glDisable(GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		glDisable(GL_FOG);
		glDisable(GL_COLOR_MATERIAL);
		po->Disable();
		glLineWidth(4.0f);
		glBegin(GL_LINES);
			glColor4f(0.0f, 0.0f, 0.0f, 1.0f); glVertex3f(p0.x,               p0.y,               p0.z              );
			glColor4f(1.0f, 1.0f, 0.0f, 1.0f); glVertex3f(p0.x + p1.x * 5.0f, p0.y + p1.y * 5.0f, p0.z + p1.z * 5.0f);
		glEnd();
		glLineWidth(1.0f);
		po->Enable();
	glPopAttrib();
}

// retrieves the height at <x, y> itself
inline void CSMFGroundDrawer::AddVertex(int x, int y, bool inShadowPass) {
	const float h = heightData[y * heightDataX + x];

	const vec3f p = vec3f(x * smfMap->SQUARE_SIZE, h, y * smfMap->SQUARE_SIZE);
	const vec3f& n = smfMap->vertexNormals[y * heightDataX + x];

	// pass at least a semi-acceptable normal for this vertex
	// rather than just its position (since we just have the
	// SMF skeleton, not the shading-texture)
	if (!inShadowPass) {
		va->AddVertexN(p, n);
	} else {
		va->AddVertex0(p);
	}
}

inline void CSMFGroundDrawer::AddVertex(int x, int y, float height, bool inShadowPass) {
	const vec3f p = vec3f(x * smfMap->SQUARE_SIZE, height, y * smfMap->SQUARE_SIZE);
	const vec3f& n = smfMap->vertexNormals[y * heightDataX + x];

	// FIXME: this is broken now
	// DrawNormal(p, n, shaderProObj);

	if (!inShadowPass) {
		va->AddVertexN(p, n);
	} else {
		va->AddVertex0(p);
	}
}




inline bool CSMFGroundDrawer::BigTexSquareRowVisible(const Camera* cam, int bty) {
	const int minx =             0;
	const int maxx =      mapWidth;
	const int minz = bty * bigTexH;
	const int maxz = minz + bigTexH;
	const float miny = readMap->minheight;
	const float maxy = fabs(cam->pos.y); // ??

	const vec3f mins(minx, miny, minz);
	const vec3f maxs(maxx, maxy, maxz);

	return (cam->InView(mins, maxs));
}

inline void CSMFGroundDrawer::FindRange(int& xs, int& xe, std::vector<fline>& left, std::vector<fline>& right, int y, int lod) {
	int xt0, xt1;
	float xtf = 0.0f;
	std::vector<fline>::iterator fli;

	for (fli = left.begin(); fli != left.end(); fli++) {
		xtf = fli->base / smfMap->SQUARE_SIZE + fli->dir * y;
		xt0 = (int(xtf)) / lod * lod - lod;
		xt1 = ((int) (xtf + fli->dir * lod)) / lod * lod - lod;

		if (xt0 > xt1) xt0 = xt1;
		if (xt0 > xs) xs = xt0;
	}
	for (fli = right.begin(); fli != right.end(); fli++) {
		xtf = fli->base / smfMap->SQUARE_SIZE + fli->dir * y;
		xt0 = (int(xtf)) / lod * lod + lod;
		xt1 = ((int) (xtf + fli->dir * lod)) / lod * lod + lod;

		if (xt0 < xt1) xt0 = xt1;
		if (xt0 < xe) xe = xt0;
	}
}


#define CLAMP(i) std::max(0, std::min(int(i), maxIdx))

inline void CSMFGroundDrawer::DoDrawGroundRow(const Camera* cam, int bty, bool inShadowPass) {
	if (!BigTexSquareRowVisible(cam, bty)) {
		// skip this entire row of squares if we can't see it
		return;
	}

	bool inStrip = false;
	float x0, x1;
	int x,y;
	int sx = 0;
	int ex = numBigTexX;
	std::vector<fline>::iterator fli;

	// only process the necessary big squares in the x direction
	int bigSquareSizeY = bty * bigSquareSize;

	for (fli = left.begin(); fli != left.end(); fli++) {
		x0 = fli->base / smfMap->SQUARE_SIZE + fli->dir * bigSquareSizeY;
		x1 = x0 + fli->dir * bigSquareSize;

		if (x0 > x1)
			x0 = x1;

		x0 /= bigSquareSize;

		if (x0 > sx)
			sx = (int) x0;
	}
	for (fli = right.begin(); fli != right.end(); fli++) {
		x0 = fli->base / smfMap->SQUARE_SIZE + fli->dir * bigSquareSizeY + bigSquareSize;
		x1 = x0 + fli->dir * bigSquareSize;

		if (x0 < x1)
			x0 = x1;

		x0 /= bigSquareSize;

		if (x0 < ex)
			ex = (int) x0;
	}

	float cx2 = cam->pos.x / smfMap->SQUARE_SIZE;
	float cy2 = cam->pos.z / smfMap->SQUARE_SIZE;

	for (int btx = sx; btx < ex; ++btx) {
		if (!inShadowPass) {
			// don't do texturing in the shadow pass
			textures->SetTexture(btx, bty);

			shaderProObj->SetUniform1i(4, btx);
			shaderProObj->SetUniform1i(5, bty);
		}

		va->Initialize();

		for (unsigned int lod = 1; lod < neededLod; lod <<= 1) {
			float oldcamxpart = 0.0f;
			float oldcamypart = 0.0f;

			int hlod = lod >> 1;
			int dlod = lod << 1;

			int cx = int(cx2);
			int cy = int(cy2);

			if (lod > 1) {
				int   cxo  = (cx  / hlod) * hlod;
				int   cyo  = (cy  / hlod) * hlod;
				float cx2o = (cxo /  lod) *  lod;
				float cy2o = (cyo /  lod) *  lod;
				oldcamxpart = (cx2 - cx2o) / lod;
				oldcamypart = (cy2 - cy2o) / lod;
			}

			cx = (cx / lod) * lod;
			cy = (cy / lod) * lod;
			int ysquaremod = (cy % dlod) / lod;
			int xsquaremod = (cx % dlod) / lod;

			float camxpart = (cx2 - ((cx / dlod) * dlod)) / dlod;
			float camypart = (cy2 - ((cy / dlod) * dlod)) / dlod;

			float mcxp  = 1.0f - camxpart;
			float hcxp  = 0.5f * camxpart;
			float hmcxp = 0.5f * mcxp;

			float mcyp  = 1.0f - camypart;
			float hcyp  = 0.5f * camypart;
			float hmcyp = 0.5f * mcyp;

			float mocxp  = 1.0f - oldcamxpart;
			float hocxp  = 0.5f * oldcamxpart;
			float hmocxp = 0.5f * mocxp;

			float mocyp  = 1.0f - oldcamypart;
			float hocyp  = 0.5f * oldcamypart;
			float hmocyp = 0.5f * mocyp;

			int minty = bty * bigSquareSize;
			int maxty = minty + bigSquareSize;
			int mintx = btx * bigSquareSize;
			int maxtx = mintx + bigSquareSize;

			int minly = cy + (-viewRadius + 3 - ysquaremod) * lod;
			int maxly = cy + ( viewRadius - 1 - ysquaremod) * lod;
			int minlx = cx + (-viewRadius + 3 - xsquaremod) * lod;
			int maxlx = cx + ( viewRadius - 1 - xsquaremod) * lod;

			int xstart = std::max(minlx, mintx);
			int xend   = std::min(maxlx, maxtx);
			int ystart = std::max(minly, minty);
			int yend   = std::min(maxly, maxty);

			int vrhlod = viewRadius * hlod;

			for (y = ystart; y < yend; y += lod) {
				int xs = xstart;
				int xe = xend;

				FindRange(xs, xe, left, right, y, lod);

				const int ylod = y + lod;
				const int yhlod = y + hlod;
				const int nloop = (xe - xs) / lod + 1;

				va->EnlargeArrays(52 * nloop, 14 * nloop + 1); //! includes one extra for final endstrip

				const int yhdx = y * heightDataX;
				const int ylhdx = yhdx + lod * heightDataX;
				const int yhhdx = yhdx + hlod * heightDataX;

				for (x = xs; x < xe; x += lod) {
					const int xlod = x + lod;
					const int xhlod = x + hlod;

					//! info: all triangle quads start in the top left corner
					if ((lod == 1) ||
						(x > cx + vrhlod) || (x < cx - vrhlod) ||
						(y > cy + vrhlod) || (y < cy - vrhlod)) {
						//! normal terrain (all vertices in one LOD)
						if (!inStrip) {
							AddVertex(x, y, inShadowPass);
							AddVertex(x, ylod, inShadowPass);
							inStrip = true;
						}

						AddVertex(xlod, y, inShadowPass);
						AddVertex(xlod, ylod, inShadowPass);
					}
					else {
						//! border between 2 different LODs
						if ((x >= cx + vrhlod)) {
							//! lower LOD to the right
							int idx1 = CLAMP(yhdx + x),  idx1LOD = CLAMP(idx1 + lod), idx1HLOD = CLAMP(idx1 + hlod);
							int idx2 = CLAMP(ylhdx + x), idx2LOD = CLAMP(idx2 + lod), idx2HLOD = CLAMP(idx2 + hlod);
							int idx3 = CLAMP(yhhdx + x),                              idx3HLOD = CLAMP(idx3 + hlod);
							float h1 = (heightData[idx1] + heightData[idx2   ]) * hmocxp + heightData[idx3    ] * oldcamxpart;
							float h2 = (heightData[idx1] + heightData[idx1LOD]) * hmocxp + heightData[idx1HLOD] * oldcamxpart;
							float h3 = (heightData[idx2] + heightData[idx1LOD]) * hmocxp + heightData[idx3HLOD] * oldcamxpart;
							float h4 = (heightData[idx2] + heightData[idx2LOD]) * hmocxp + heightData[idx2HLOD] * oldcamxpart;

							if (inStrip) {
								va->EndStripQ();
								inStrip = false;
							}

							AddVertex(x, y, inShadowPass);
							AddVertex(x, yhlod, h1, inShadowPass);
							AddVertex(xhlod, y, h2, inShadowPass);
							AddVertex(xhlod, yhlod, h3, inShadowPass);
							va->EndStripQ();
							AddVertex(x, yhlod, h1, inShadowPass);
							AddVertex(x, ylod, inShadowPass);
							AddVertex(xhlod, yhlod, h3, inShadowPass);
							AddVertex(xhlod, ylod, h4, inShadowPass);
							va->EndStripQ();
							AddVertex(xhlod, ylod, h4, inShadowPass);
							AddVertex(xlod, ylod, inShadowPass);
							AddVertex(xhlod, yhlod, h3, inShadowPass);
							AddVertex(xlod, y, inShadowPass);
							AddVertex(xhlod, y, h2, inShadowPass);
							va->EndStripQ();
						}
						else if ((x <= cx - vrhlod)) {
							//! lower LOD to the left
							int idx1 = CLAMP(yhdx + x),  idx1LOD = CLAMP(idx1 + lod), idx1HLOD = CLAMP(idx1 + hlod);
							int idx2 = CLAMP(ylhdx + x), idx2LOD = CLAMP(idx2 + lod), idx2HLOD = CLAMP(idx2 + hlod);
							int idx3 = CLAMP(yhhdx + x), idx3LOD = CLAMP(idx3 + lod), idx3HLOD = CLAMP(idx3 + hlod);
							float h1 = (heightData[idx1LOD] + heightData[idx2LOD]) * hocxp + heightData[idx3LOD ] * mocxp;
							float h2 = (heightData[idx1   ] + heightData[idx1LOD]) * hocxp + heightData[idx1HLOD] * mocxp;
							float h3 = (heightData[idx2   ] + heightData[idx1LOD]) * hocxp + heightData[idx3HLOD] * mocxp;
							float h4 = (heightData[idx2   ] + heightData[idx2LOD]) * hocxp + heightData[idx2HLOD] * mocxp;

							if (inStrip) {
								va->EndStripQ();
								inStrip = false;
							}

							AddVertex(xlod, yhlod, h1, inShadowPass);
							AddVertex(xlod, y, inShadowPass);
							AddVertex(xhlod, yhlod, h3, inShadowPass);
							AddVertex(xhlod, y, h2, inShadowPass);
							va->EndStripQ();
							AddVertex(xlod, ylod, inShadowPass);
							AddVertex(xlod, yhlod, h1, inShadowPass);
							AddVertex(xhlod, ylod, h4, inShadowPass);
							AddVertex(xhlod, yhlod, h3, inShadowPass);
							va->EndStripQ();
							AddVertex(xhlod, y, h2, inShadowPass);
							AddVertex(x, y, inShadowPass);
							AddVertex(xhlod, yhlod, h3, inShadowPass);
							AddVertex(x, ylod, inShadowPass);
							AddVertex(xhlod, ylod, h4, inShadowPass);
							va->EndStripQ();
						}

						if ((y >= cy + vrhlod)) {
							//! lower LOD above
							int idx1 = yhdx + x,  idx1LOD = CLAMP(idx1 + lod), idx1HLOD = CLAMP(idx1 + hlod);
							int idx2 = ylhdx + x, idx2LOD = CLAMP(idx2 + lod);
							int idx3 = yhhdx + x, idx3LOD = CLAMP(idx3 + lod), idx3HLOD = CLAMP(idx3 + hlod);
							float h1 = (heightData[idx1   ] + heightData[idx1LOD]) * hmocyp + heightData[idx1HLOD] * oldcamypart;
							float h2 = (heightData[idx1   ] + heightData[idx2   ]) * hmocyp + heightData[idx3    ] * oldcamypart;
							float h3 = (heightData[idx2   ] + heightData[idx1LOD]) * hmocyp + heightData[idx3HLOD] * oldcamypart;
							float h4 = (heightData[idx2LOD] + heightData[idx1LOD]) * hmocyp + heightData[idx3LOD ] * oldcamypart;

							if (inStrip) {
								va->EndStripQ();
								inStrip = false;
							}

							AddVertex(x, y, inShadowPass);
							AddVertex(x, yhlod, h2, inShadowPass);
							AddVertex(xhlod, y, h1, inShadowPass);
							AddVertex(xhlod, yhlod, h3, inShadowPass);
							AddVertex(xlod, y, inShadowPass);
							AddVertex(xlod, yhlod, h4, inShadowPass);
							va->EndStripQ();
							AddVertex(x, yhlod, h2, inShadowPass);
							AddVertex(x, ylod, inShadowPass);
							AddVertex(xhlod, yhlod, h3, inShadowPass);
							AddVertex(xlod, ylod, inShadowPass);
							AddVertex(xlod, yhlod, h4, inShadowPass);
							va->EndStripQ();
						}
						else if ((y <= cy - vrhlod)) {
							//! lower LOD beneath
							int idx1 = CLAMP(yhdx + x),  idx1LOD = CLAMP(idx1 + lod);
							int idx2 = CLAMP(ylhdx + x), idx2LOD = CLAMP(idx2 + lod), idx2HLOD = CLAMP(idx2 + hlod);
							int idx3 = CLAMP(yhhdx + x), idx3LOD = CLAMP(idx3 + lod), idx3HLOD = CLAMP(idx3 + hlod);
							float h1 = (heightData[idx2   ] + heightData[idx2LOD]) * hocyp + heightData[idx2HLOD] * mocyp;
							float h2 = (heightData[idx1   ] + heightData[idx2   ]) * hocyp + heightData[idx3    ] * mocyp;
							float h3 = (heightData[idx2   ] + heightData[idx1LOD]) * hocyp + heightData[idx3HLOD] * mocyp;
							float h4 = (heightData[idx2LOD] + heightData[idx1LOD]) * hocyp + heightData[idx3LOD ] * mocyp;

							if (inStrip) {
								va->EndStripQ();
								inStrip = false;
							}

							AddVertex(x, yhlod, h2, inShadowPass);
							AddVertex(x, ylod, inShadowPass);
							AddVertex(xhlod, yhlod, h3, inShadowPass);
							AddVertex(xhlod, ylod, h1, inShadowPass);
							AddVertex(xlod, yhlod, h4, inShadowPass);
							AddVertex(xlod, ylod, inShadowPass);
							va->EndStripQ();
							AddVertex(xlod, yhlod, h4, inShadowPass);
							AddVertex(xlod, y, inShadowPass);
							AddVertex(xhlod, yhlod, h3, inShadowPass);
							AddVertex(x, y, inShadowPass);
							AddVertex(x, yhlod, h2, inShadowPass);
							va->EndStripQ();
						}
					}
				}

				if (inStrip) {
					va->EndStripQ();
					inStrip = false;
				}
			}

			const int yst = std::max(ystart - int(lod), minty);
			const int yed = std::min(yend + int(lod), maxty);
			const int nloop = (yed - yst) / lod + 1;

			va->EnlargeArrays(8 * nloop, 2 * nloop);

			if (maxlx < maxtx && maxlx >= mintx) {
				x = maxlx;
				int xlod = x + lod;
				for (y = yst; y < yed; y += lod) {
					AddVertex(x, y, inShadowPass);
					AddVertex(x, y + lod, inShadowPass);

					if (y % dlod) {
						int idx1 = CLAMP((y      ) * heightDataX + x), idx1LOD = CLAMP(idx1 + lod);
						int idx2 = CLAMP((y + lod) * heightDataX + x), idx2LOD = CLAMP(idx2 + lod);
						int idx3 = CLAMP((y - lod) * heightDataX + x), idx3LOD = CLAMP(idx3 + lod);
						float h = (heightData[idx3LOD] + heightData[idx2LOD]) * hmcxp +	heightData[idx1LOD] * camxpart;

						AddVertex(xlod, y, h, inShadowPass);
						AddVertex(xlod, y + lod, inShadowPass);
					} else {
						int idx1 = CLAMP((y       ) * heightDataX + x), idx1LOD = CLAMP(idx1 + lod);
						int idx2 = CLAMP((y +  lod) * heightDataX + x), idx2LOD = CLAMP(idx2 + lod);
						int idx3 = CLAMP((y + dlod) * heightDataX + x), idx3LOD = CLAMP(idx3 + lod);
						float h = (heightData[idx1LOD] + heightData[idx3LOD]) * hmcxp + heightData[idx2LOD] * camxpart;

						AddVertex(xlod, y, inShadowPass);
						AddVertex(xlod, y + lod, h, inShadowPass);
					}
					va->EndStripQ();
				}
			}

			if (minlx > mintx && minlx < maxtx) {
				x = minlx - lod;
				int xlod = x + lod;
				for (y = yst; y < yed; y += lod) {
					if (y % dlod) {
						int idx1 = CLAMP((y      ) * heightDataX + x);
						int idx2 = CLAMP((y + lod) * heightDataX + x);
						int idx3 = CLAMP((y - lod) * heightDataX + x);
						float h = (heightData[idx3] + heightData[idx2]) * hcxp + heightData[idx1] * mcxp;

						AddVertex(x, y, h, inShadowPass);
						AddVertex(x, y + lod, inShadowPass);
					} else {
						int idx1 = CLAMP((y       ) * heightDataX + x);
						int idx2 = CLAMP((y +  lod) * heightDataX + x);
						int idx3 = CLAMP((y + dlod) * heightDataX + x);
						float h = (heightData[idx1] + heightData[idx3]) * hcxp + heightData[idx2] * mcxp;

						AddVertex(x, y, inShadowPass);
						AddVertex(x, y + lod, h, inShadowPass);
					}
					AddVertex(xlod, y, inShadowPass);
					AddVertex(xlod, y + lod, inShadowPass);
					va->EndStripQ();
				}
			}

			if (maxly < maxty && maxly > minty) {
				y = maxly;
				int xs = std::max(xstart - int(lod), mintx);
				int xe = std::min(xend + int(lod),   maxtx);

				FindRange(xs, xe, left, right, y, lod);

				if (xs < xe) {
					x = xs;
					const int ylod = y + lod;
					const int nloop = (xe - xs) / lod + 2; //! one extra for if statment
					const int ylhdx = (y + lod) * heightDataX;

					va->EnlargeArrays(2 * nloop, 1);

					if (x % dlod) {
						int idx2 = CLAMP(ylhdx + x), idx2PLOD = CLAMP(idx2 + lod), idx2MLOD = CLAMP(idx2 - lod);
						float h = (heightData[idx2MLOD] + heightData[idx2PLOD]) * hmcyp + heightData[idx2] * camypart;

						AddVertex(x, y, inShadowPass);
						AddVertex(x, ylod, h, inShadowPass);
					} else {
						AddVertex(x, y, inShadowPass);
						AddVertex(x, ylod, inShadowPass);
					}
					for (x = xs; x < xe; x += lod) {
						if (x % dlod) {
							AddVertex(x + lod, y, inShadowPass);
							AddVertex(x + lod, ylod, inShadowPass);
						}
						else {
							int idx2 = CLAMP(ylhdx + x), idx2PLOD  = CLAMP(idx2 +  lod), idx2PLOD2 = CLAMP(idx2 + dlod);
							float h = (heightData[idx2PLOD2] + heightData[idx2]) * hmcyp + heightData[idx2PLOD] * camypart;

							AddVertex(x + lod, y, inShadowPass);
							AddVertex(x + lod, ylod, h, inShadowPass);
						}
					}
					va->EndStripQ();
				}
			}

			if (minly > minty && minly < maxty) {
				y = minly - lod;
				int xs = std::max(xstart - int(lod), mintx);
				int xe = std::min(xend + int(lod),   maxtx);

				FindRange(xs, xe, left, right, y, lod);

				if (xs < xe) {
					x = xs;
					const int ylod = y + lod;
					const int nloop = (xe - xs) / lod + 2; //! one extra for if statment
					const int yhdx = y * heightDataX;

					va->EnlargeArrays(2 * nloop, 1);

					if (x % dlod) {
						int idx1 = CLAMP(yhdx + x), idx1PLOD = CLAMP(idx1 + lod), idx1MLOD = CLAMP(idx1 - lod);
						float h = (heightData[idx1MLOD] + heightData[idx1PLOD]) * hcyp + heightData[idx1] * mcyp;

						AddVertex(x, y, h, inShadowPass);
						AddVertex(x, ylod, inShadowPass);
					} else {
						AddVertex(x, y, inShadowPass);
						AddVertex(x, ylod, inShadowPass);
					}

					for (x = xs; x < xe; x+= lod) {
						if (x % dlod) {
							AddVertex(x + lod, y, inShadowPass);
							AddVertex(x + lod, ylod, inShadowPass);
						} else {
							int idx1 = CLAMP(yhdx + x), idx1PLOD  = CLAMP(idx1 +  lod), idx1PLOD2 = CLAMP(idx1 + dlod);
							float h = (heightData[idx1PLOD2] + heightData[idx1]) * hcyp + heightData[idx1PLOD] * mcyp;

							AddVertex(x + lod, y, h, inShadowPass);
							AddVertex(x + lod, ylod, inShadowPass);
						}
					}

					va->EndStripQ();
				}
			}
		}

		// we are passing our own normals in AddVertex() and
		// tex-coors are generated per-vertex by SetTexGen(),
		// so use DrawArrayN (position + normals only)
		if (!inShadowPass) {
			va->DrawArrayN(GL_TRIANGLE_STRIP);
		} else {
			va->DrawArray0(GL_TRIANGLE_STRIP);
		}
	}
}



// the water renderers call this in their DrawReflection() and DrawRefraction() funcs
//    CBFGroundDrawer::Draw(drawWaterReflection 0, drawUnitReflection 0, overrideVP 0) basic pass (CGame::DrawWorld())
//    CBFGroundDrawer::Draw(drawWaterReflection 0, drawUnitReflection 1, overrideVP 0) refl. pass
//    CBFGroundDrawer::Draw(drawWaterReflection 1, drawUnitReflection 0, overrideVP 0) refr. pass
void CSMFGroundDrawer::Draw(const Camera* cam, bool inShadowPass) {
	if (wireframe) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else {
		glPolygonMode(GL_FRONT, GL_FILL);
	}

	/// viewRadius = std::max(std::max(numBigTexY, numBigTexX), viewRadius);
	/// viewRadius = int(viewRadius * sqrtf(45.0f / cam->vFOV));
	/// viewRadius += (viewRadius & 1);
	neededLod = int((guViewRange * 0.125f) / viewRadius) << 1;


	UpdateCamRestraints(cam);

	if (!inShadowPass) {
		shaderProObj->Enable();
		shaderProObj->SetUniformMatrix4fv(3, false, const_cast<float*>(shadowHandler->GetShadowProjectionMatrix()));

		glEnable(GL_TEXTURE_2D);

		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, shadowHandler->GetDepthTextureID());
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, overlayTexID);
		glActiveTexture(GL_TEXTURE0);
		textures->DrawUpdate(cam);
	}

	for (int bty = 0; bty < numBigTexY; ++bty) {
		DoDrawGroundRow(cam, bty, inShadowPass);
	}

	if (!inShadowPass) {
		glActiveTexture(GL_TEXTURE7); glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, 0);

		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glDisable(GL_TEXTURE_2D);

		shaderProObj->Disable();
	}

	if (wireframe) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
}






void CSMFGroundDrawer::AddFrustumRestraint(const vec3f& pos, const vec3f& side) {
	fline line;

	// get vector for collision between frustum and horizontal plane
	vec3f b = YVECf.cross(side);

	if (fabs(b.z) < 0.0001f)
		b.z = 0.0001f;

	{
		line.dir = b.x / b.z;				// set direction to that
		vec3f c = b.cross(side);			// get vector from camera to collision line
		vec3f colpoint;						// point on the collision line

		if (side.y > 0.0f) {
			colpoint = pos - c * ((pos.y - (readMap->minheight - 100)) / c.y);
		} else {
			colpoint = pos - c * ((pos.y - (readMap->maxheight +  30)) / c.y);
		}

		// get intersection between colpoint and z axis
		line.base = colpoint.x - colpoint.z * line.dir;

		if (b.z > 0.0f) {
			left.push_back(line);
		} else {
			right.push_back(line);
		}
	}
}

void CSMFGroundDrawer::UpdateCamRestraints(const Camera* cam) {
	left.clear();
	right.clear();

	// add restraints for camera sides
	AddFrustumRestraint(cam->pos, cam->frustumB);
	AddFrustumRestraint(cam->pos, cam->frustumT);
	AddFrustumRestraint(cam->pos, cam->frustumR);
	AddFrustumRestraint(cam->pos, cam->frustumL);

	// add restraint for maximum view distance
	fline line;
	vec3f side = cam->zdir;
	vec3f camHor = side;
		camHor.y = 0.0f;
		camHor.inorm3D();

	// get vector for collision between frustum and horizontal plane
	vec3f b = YVECf.cross(camHor);

	if (fabs(b.z) > 0.0001f) {
		line.dir = b.x / b.z;			// set direction to that
		vec3f c = b.cross(camHor);		// get vector from camera to collision line
		vec3f colpoint;					// point on the collision line

		if (side.y > 0.0f) {
			colpoint = cam->pos + camHor * guViewRange * 1.05f - c * (cam->pos.y / c.y);
		} else {
			colpoint = cam->pos + camHor * guViewRange * 1.05f - c * ((cam->pos.y - 255 / 3.5f) / c.y);
		}

		// get intersection between colpoint and z axis
		line.base = colpoint.x - colpoint.z * line.dir;

		if (b.z > 0) {
			left.push_back(line);
		} else {
			right.push_back(line);
		}
	}
}
