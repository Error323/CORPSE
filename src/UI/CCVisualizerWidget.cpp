#include <limits>

#include <SDL/SDL.h>
#include <GL/gl.h>

#include "./CCVisualizerWidget.hpp"
#include "../Map/BaseGroundDrawer.hpp"
#include "../Map/Ground.hpp"
#include "../Map/ReadMap.hpp"
#include "../Math/vec3.hpp"
#include "../Renderer/CameraController.hpp"
#include "../Renderer/Camera.hpp"
#include "../Renderer/RenderThread.hpp"
#include "../Renderer/VertexArray.hpp"
#include "../Sim/SimThread.hpp"

ui::CCVisualizerWidget::~CCVisualizerWidget() {
	for (std::map<unsigned int, TextureOverlay*>::iterator it = textureOverlays.begin(); it != textureOverlays.end(); ++it) {
		delete it->second;
	}
	for (std::map<unsigned int, VectorOverlay*>::iterator it = vectorOverlays.begin(); it != vectorOverlays.end(); ++it) {
		delete it->second;
	}

	textureOverlays.clear();
	vectorOverlays.clear();
}

void ui::CCVisualizerWidget::KeyPressed(int key) {
	if (key == SDLK_v) { enabled = !enabled; }
	if (!enabled) { return; }

	if (mModule == NULL) {
		mModule = sThread->GetPathModule();
	}

	unsigned int dataType = mModule->GetNumScalarDataTypes() + mModule->GetNumVectorDataTypes();

	if (key >= SDLK_0 && key <= SDLK_9) {
		dataType = key - SDLK_0;
	} else {
		return;
	}

	// all parameters except <type> and <group> are filled by the Get*DataTypeInfo() call
	IPathModule::DataTypeInfo info = DATATYPEINFO_RWRITE;

	if (dataType < mModule->GetNumScalarDataTypes()) {
		// scalar field; create a texture overlay
		TextureOverlay* textureOverlay = textureOverlays[dataType];
		CBaseGroundDrawer* groundDrawer = readMap->GetGroundDrawer();

		info.type = dataType;
		info.group = texVisGroupID;

		if (textureOverlay == NULL) {
			mModule->GetScalarDataTypeInfo(&info);

			if (!info.global) {
				SetNextVisGroupID(true);
			}

			textureOverlay = new TextureOverlay(&info);
			textureOverlays[dataType] = textureOverlay;
			currentTextureOverlay = textureOverlay;

			groundDrawer->SetOverlayTexture(textureOverlay->GetID());
		} else {
			if (!textureOverlay->IsEnabled()) {
				textureOverlay->SetEnabled(true);
				groundDrawer->SetOverlayTexture(textureOverlay->GetID());

				if (!textureOverlay->IsGlobal()) {
					SetNextVisGroupID(true);
				}

				currentTextureOverlay = textureOverlay;
			} else {
				if (textureOverlay->IsGlobal() || !SetNextVisGroupID(true)) {
					textureOverlay->SetEnabled(false);
					groundDrawer->SetOverlayTexture(0);

					currentTextureOverlay = NULL;
				}
			}
		}
	} else {
		// vector field, create a vertex overlay
		VectorOverlay* vectorOverlay = vectorOverlays[dataType];

		info.type = dataType;
		info.group = vecVisGroupID;

		if (vectorOverlay == NULL) {
			mModule->GetVectorDataTypeInfo(&info);

			if (!info.global) {
				SetNextVisGroupID(false);
			}

			vectorOverlay = new VectorOverlay(&info);
			vectorOverlays[dataType] = vectorOverlay;
			currentVectorOverlay = vectorOverlay;
		} else {
			if (!vectorOverlay->IsEnabled()) {
				vectorOverlay->SetEnabled(true);

				if (!vectorOverlay->IsGlobal()) {
					SetNextVisGroupID(false);
				}

				currentVectorOverlay = vectorOverlay;
			} else {
				if (vectorOverlay->IsGlobal() || !SetNextVisGroupID(false)) {
					vectorOverlay->SetEnabled(false);

					currentVectorOverlay = NULL;
				}
			}
		}
	}
}

bool ui::CCVisualizerWidget::SetNextVisGroupID(bool texture) {
	// cycle to the next groupID for the non-global overlays
	const unsigned int numGroupIDs = mModule->GetNumGroupIDs();

	if (numGroupIDs == 0) {
		return false;
	}

	// allow disabling active overlays when the
	// current index exceeds the number of IDs
	if (texture) {
		if (texVisGroupIdx >= numGroupIDs) {
			texVisGroupIdx = 0;
			return false;
		}
	} else {
		if (vecVisGroupIdx >= numGroupIDs) {
			vecVisGroupIdx = 0;
			return false;
		}
	}

	// get the current group ID's
	visGroupIDs.resize(numGroupIDs);
	mModule->GetGroupIDs(&visGroupIDs[0], numGroupIDs);

	if (texture) {
		texVisGroupID = visGroupIDs[texVisGroupIdx++];
	} else {
		vecVisGroupID = visGroupIDs[vecVisGroupIdx++];
	}

	return true;
}

void ui::CCVisualizerWidget::Update(const vec3i&, const vec3i&) {
	static unsigned int simFrame = sThread->GetFrame();
	static IPathModule::DataTypeInfo info = DATATYPEINFO_RWRITE;

	if (!enabled) {
		return;
	}

	if (sThread->GetFrame() != simFrame) {
		simFrame = sThread->GetFrame();

		if (currentTextureOverlay != NULL && currentTextureOverlay->IsEnabled()) {
			// if the group corresponding to <visGroupID> no longer exists,
			// this causes the texture to be filled with default values (0)
			info.type = currentTextureOverlay->GetDataType();
			info.group = texVisGroupID;

			mModule->GetScalarDataTypeInfo(&info);
			currentTextureOverlay->Update(info.fdata);
		}

		if (currentVectorOverlay != NULL && currentVectorOverlay->IsEnabled()) {
			info.type = currentVectorOverlay->GetDataType();
			info.group = vecVisGroupID;

			mModule->GetVectorDataTypeInfo(&info);
			currentVectorOverlay->Update(info.vdata);
		}
	}

	if (currentVectorOverlay != NULL && currentVectorOverlay->IsEnabled()) {
		Camera* camera = rThread->GetCamCon()->GetCurrCam();

		glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
		glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
		glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT);
			glEnable(GL_DEPTH_TEST);
			glLineWidth(2.0f);
			camera->ApplyViewProjTransform();
			currentVectorOverlay->Draw();
		glPopAttrib();
		glMatrixMode(GL_PROJECTION); glPopMatrix();
		glMatrixMode(GL_MODELVIEW); glPopMatrix();
	}
}






ui::CCVisualizerWidget::Overlay::Overlay(const IPathModule::DataTypeInfo* info):
	global(info->global),
	enabled(true),
	sizex(info->sizex),
	sizey(info->sizey),
	stride(info->stride),
	dataType(info->type) {
}

ui::CCVisualizerWidget::TextureOverlay::TextureOverlay(const IPathModule::DataTypeInfo* info):
	Overlay(info),
	id(0),
	data(new unsigned char[info->sizex * info->sizey * 4]) // bpp == 4
{
	memset(data, 0, sizex * sizey * 4);
	Update(info->fdata);
}

ui::CCVisualizerWidget::TextureOverlay::~TextureOverlay() {
	glDeleteTextures(1, &id); id = 0;
	delete[] data; data = NULL;
}

void ui::CCVisualizerWidget::TextureOverlay::Update(const float* ndata) {
	// note: ndata only represents one channel if stride == 1
	static const int intFormat = GL_RGBA;
	static const int extFormat = GL_RGBA;
	static const int dataType  = GL_UNSIGNED_BYTE;
	static const int bpp       = 4;

	const unsigned int srcArraySize = sizex * sizey * stride;
	const unsigned int dstArraySize = sizex * sizey * bpp;

	static float ndataMin[bpp] = {0.0f};
	static float ndataMax[bpp] = {0.0f};

	for (int i = 0; i < bpp; i++) {
		ndataMin[i] =  std::numeric_limits<float>::max();
		ndataMax[i] = -std::numeric_limits<float>::max();
	}

	if (ndata != NULL) {
		// find the extrema and normalize
		switch (stride) {
			case 1: {
				for (unsigned int i = 0; i < srcArraySize; i += stride) {
					ndataMin[0] = std::min(ndataMin[0], ndata[i]);
					ndataMax[0] = std::max(ndataMax[0], ndata[i]);
				}

				// one float scalar value per cell, visualize these as
				// <R, 0, 0, 255> (if positive) or as <0, 0, B, 255>
				// (if negative)
				for (unsigned int i = 0; i < srcArraySize; i += stride) {
					data[(i * bpp) + 0] = (ndata[i] < 0.0f)? 0: ((ndata[i] / ndataMax[0]) * 255);
					data[(i * bpp) + 1] =                                                     0;
					data[(i * bpp) + 2] = (ndata[i] > 0.0f)? 0: ((ndata[i] / ndataMin[0]) * 255);
					data[(i * bpp) + 3] =                                                   255;
				}
			} break;

			case 4: {
				for (unsigned int i = 0; i < srcArraySize; i += stride) {
					ndataMin[0] = std::min(ndataMin[0], ndata[i + 0]), ndataMax[0] = std::max(ndataMax[0], ndata[i + 0]);
					ndataMin[1] = std::min(ndataMin[1], ndata[i + 1]), ndataMax[1] = std::max(ndataMax[1], ndata[i + 1]);
					ndataMin[2] = std::min(ndataMin[2], ndata[i + 2]), ndataMax[2] = std::max(ndataMax[2], ndata[i + 2]);
					ndataMin[3] = std::min(ndataMin[3], ndata[i + 3]), ndataMax[3] = std::max(ndataMax[3], ndata[i + 3]);
				}

				// four float scalar values per cell, visualize these as
				// <R, G, B, A> individually normalized per channel (the
				// A value is problematic, and this normalization scheme
				// also precludes showing negative field elements)
				for (unsigned int i = 0; i < srcArraySize; i += stride) {
					data[i + 0] = ((ndata[i + 0] - ndataMin[0]) / (ndataMax[0] - ndataMin[0])) * 255;
					data[i + 1] = ((ndata[i + 1] - ndataMin[1]) / (ndataMax[1] - ndataMin[1])) * 255;
					data[i + 2] = ((ndata[i + 2] - ndataMin[2]) / (ndataMax[2] - ndataMin[2])) * 255;
					data[i + 3] = ((ndata[i + 3] - ndataMin[3]) / (ndataMax[3] - ndataMin[3])) * 255;
				}
			} break;

			default: {
			} break;
		}
	} else {
		for (unsigned int i = 0; i < dstArraySize; i += bpp) {
			data[i + 0] =   0;
			data[i + 1] =   0;
			data[i + 2] =   0;
			data[i + 3] = 255;
		}
	}

	if (id != 0) {
		// update old texture
		glBindTexture(GL_TEXTURE_2D, id);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, sizex, sizey, intFormat, dataType, data);
		glBindTexture(GL_TEXTURE_2D, 0);
	} else {
		// create new texture
		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, intFormat, sizex, sizey, 0, extFormat, dataType, data);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}



ui::CCVisualizerWidget::VectorOverlay::VectorOverlay(const IPathModule::DataTypeInfo* info):
	Overlay(info),
	data(new VertexArray())
{
	data->EnlargeArrays((sizex * sizey * stride) * 2, 0, VA_SIZE_C);
	Update(info->vdata);
}

ui::CCVisualizerWidget::VectorOverlay::~VectorOverlay() {
	delete data;
}

void ui::CCVisualizerWidget::VectorOverlay::Draw() {
	data->DrawArrayC(GL_LINES);
}

void ui::CCVisualizerWidget::VectorOverlay::Update(const vec3f* ndata) {
	static const unsigned char dirColors[5][4] = {
		{255,   0,   0, 255}, // N (red)
		{  0, 255,   0, 255}, // S (green)
		{  0,   0, 255, 255}, // E (blue)
		{255, 255,   0, 255}, // W (yellow)
		{255, 255, 255, 255}, // M (white)
	};

	if (ndata != NULL) {
		data->Initialize();

		const unsigned int numVectors = sizex * sizey * stride;
		// const unsigned int numVertices = numVectors * 2;
		// get the grid's downscale-factors (G2Hx == G2Hz)
		const unsigned int G2Hx = readMap->mapx / sizex;
		const unsigned int G2Hz = readMap->mapy / sizey;
		const unsigned int H2W  = readMap->SQUARE_SIZE;

		switch (stride) {
			case 1: {
				for (unsigned int i = 0; i < numVectors; i += stride) {
					const vec3f& v = ndata[i];

					const unsigned int c =
						(v.x >= 0.0f && v.z <  0.0f)? 0: // "N" quadrant
						(v.x >= 0.0f && v.z >= 0.0f)? 1: // "S" quadrant
						(v.x <  0.0f && v.z >= 0.0f)? 2: // "E" quadrant
						(v.x <  0.0f && v.z <  0.0f)? 3: // "W" quadrant
						4;
					const unsigned int gx = i % sizex;
					const unsigned int gz = i / sizex;
					const float wx = (gx * G2Hx * H2W) + ((G2Hx * H2W) >> 1);
					const float wz = (gz * G2Hz * H2W) + ((G2Hz * H2W) >> 1);
					const float wy = ground->GetHeight(wx, wz);
					const float s = v.len3D();

					data->AddVertexQC(vec3f(wx,           wy + (G2Hx * H2W), wz          ), dirColors[4]);
					data->AddVertexQC(vec3f(wx + v.x * s, wy + (G2Hx * H2W), wz + v.z * s), dirColors[c]);
				}
			} break;

			case 4: {
				for (unsigned int i = 0; i < numVectors; i += stride) {
					// note: size of ndata must be a multiple of stride
					const vec3f& vN = ndata[i + 0];
					const vec3f& vS = ndata[i + 1];
					const vec3f& vE = ndata[i + 2];
					const vec3f& vW = ndata[i + 3];

					// if stride equals 4, then
					//    vectors 0 - 3 belong to cell 0
					//    vectors 4 - 7 belong to cell 1
					//    vectors 8 -11 belong to cell 2
					// so cell is given by i / stride
					const unsigned int j = i / stride;
					const unsigned int gx = j % sizex;
					const unsigned int gz = j / sizex;

					// get the world-space coordinates of the cell's center
					const float wx = (gx * G2Hx * H2W) + ((G2Hx * H2W) >> 1);
					const float wz = (gz * G2Hz * H2W) + ((G2Hz * H2W) >> 1);
					const float wy = ground->GetHeight(wx, wz);

					data->AddVertexQC(vec3f(wx,         wy + H2W, wz       ), dirColors[4]);
					data->AddVertexQC(vec3f(wx + vN.x,  wy + H2W, wz + vN.z), dirColors[0]);

					data->AddVertexQC(vec3f(wx,         wy + H2W, wz       ), dirColors[4]);
					data->AddVertexQC(vec3f(wx + vS.x,  wy + H2W, wz + vS.z), dirColors[1]);

					data->AddVertexQC(vec3f(wx,         wy + H2W, wz       ), dirColors[4]);
					data->AddVertexQC(vec3f(wx + vE.x,  wy + H2W, wz + vE.z), dirColors[2]);

					data->AddVertexQC(vec3f(wx,         wy + H2W, wz       ), dirColors[4]);
					data->AddVertexQC(vec3f(wx + vW.x,  wy + H2W, wz + vW.z), dirColors[3]);
				}
			} break;

			default: {
			} break;
		}
	}
}
