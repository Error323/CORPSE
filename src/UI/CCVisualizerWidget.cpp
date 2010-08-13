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
#include "../Path/IPathModule.hpp"

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
		mModule = simThread->GetPathModule();
	}

	unsigned int dataType = mModule->GetNumScalarDataTypes() + mModule->GetNumVectorDataTypes();

	if (key >= SDLK_0 && key <= SDLK_9) {
		dataType = key - SDLK_0;
	} else {
		return;
	}

	printf("[CCVisWidget::KeyPress] dataType=%u, visGroupIdx=%u, visGroupID=%u\n", dataType, visGroupIdx, visGroupID);

	if (dataType < mModule->GetNumScalarDataTypes()) {
		// scalar field; create a texture
		TextureOverlay* textureOverlay = textureOverlays[dataType];
		CBaseGroundDrawer* g = readMap->GetGroundDrawer();

		if (textureOverlay == NULL) {
			const unsigned int xsize = mModule->GetScalarDataArraySizeX(dataType);
			const unsigned int zsize = mModule->GetScalarDataArraySizeZ(dataType);
			const unsigned int stride = mModule->GetScalarDataArrayStride(dataType);
			const float* data = mModule->GetScalarDataArray(dataType, visGroupID);

			textureOverlay = new TextureOverlay(xsize, zsize, stride, dataType, data);
			textureOverlays[dataType] = textureOverlay;
			currentTextureOverlay = textureOverlay;

			g->SetOverlayTexture(textureOverlay->GetID());
		} else {
			const bool isGlobalOverlay = mModule->IsGlobalDataType(dataType);
			const bool toggleGlobalOverlay = (!textureOverlay->IsEnabled() && isGlobalOverlay);

			if (toggleGlobalOverlay || (!isGlobalOverlay && SetNextVisGroupID())) {
				textureOverlay->SetEnabled(true);
				g->SetOverlayTexture(textureOverlay->GetID());

				currentTextureOverlay = textureOverlay;
			} else {
				textureOverlay->SetEnabled(false);
				g->SetOverlayTexture(0);

				currentTextureOverlay = NULL;
			}
		}
	} else {
		// vector field, create a vertex array
		VectorOverlay* vectorOverlay = vectorOverlays[dataType];

		if (vectorOverlay == NULL) {
			const unsigned int xsize = mModule->GetVectorDataArraySizeX(dataType);
			const unsigned int zsize = mModule->GetVectorDataArraySizeZ(dataType);
			const unsigned int stride = mModule->GetVectorDataArrayStride(dataType);
			const vec3f* data = mModule->GetVectorDataArray(dataType, visGroupID);

			vectorOverlay = new VectorOverlay(xsize, zsize, stride, dataType, data);
			vectorOverlays[dataType] = vectorOverlay;
			currentVectorOverlay = vectorOverlay;
		} else {
			const bool isGlobalOverlay = mModule->IsGlobalDataType(dataType);
			const bool toggleGlobalOverlay = (!vectorOverlay->IsEnabled() && isGlobalOverlay);

			if (toggleGlobalOverlay || (!isGlobalOverlay && SetNextVisGroupID())) {
				vectorOverlay->SetEnabled(true);

				currentVectorOverlay = vectorOverlay;
			} else {
				vectorOverlay->SetEnabled(false);
				currentVectorOverlay = NULL;
			}
		}
	}
}

bool ui::CCVisualizerWidget::SetNextVisGroupID() {
	// cycle to the next groupID for the non-global overlays
	const unsigned int numGroupIDs = mModule->GetNumGroupIDs();

	if (numGroupIDs == 0) {
		return false;
	}

	// get the current group ID's
	visGroupIDs.resize(numGroupIDs);
	mModule->GetGroupIDs(&visGroupIDs[0], numGroupIDs);

	// allow disabling active overlays when the
	// current index exceeds the number of IDs
	if (visGroupIdx >= numGroupIDs) {
		visGroupIdx = 0;
		return false;
	}

	visGroupID = visGroupIDs[visGroupIdx++];
	return true;
}

void ui::CCVisualizerWidget::Update(const vec3i&, const vec3i&) {
	static unsigned int simFrame = simThread->GetFrame();

	if (!enabled) {
		return;
	}

	if (simThread->GetFrame() != simFrame) {
		simFrame = simThread->GetFrame();

		if (currentTextureOverlay != NULL && currentTextureOverlay->IsEnabled()) {
			// update the texture data
			// if the group corresponding to <visGroupID> no longer exists,
			// this causes the texture to be filled with default values (0)
			currentTextureOverlay->Update(mModule->GetScalarDataArray(currentTextureOverlay->GetDataType(), visGroupID));
		}

		if (currentVectorOverlay != NULL) {
			currentVectorOverlay->Update(mModule->GetVectorDataArray(currentVectorOverlay->GetDataType(), visGroupID));
		}
	}

	if (currentVectorOverlay != NULL && currentVectorOverlay->IsEnabled()) {
		Camera* camera = renderThread->GetCamCon()->GetCurrCam();

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






ui::CCVisualizerWidget::TextureOverlay::TextureOverlay(
	unsigned int x,
	unsigned int y,
	unsigned int s,
	unsigned int dt,
	const float* ndata
):
	Overlay(x, y, s, dt),
	id(0),
	#ifdef TEXTURE_DATATYPE_FLOAT
	data(new float[sizex * sizey * stride])
	#else
	data(new unsigned char[sizex * sizey * stride * 4])
	#endif
{
	Update(ndata);
}

ui::CCVisualizerWidget::TextureOverlay::~TextureOverlay() {
	glDeleteTextures(1, &id); id = 0;
	delete[] data; data = NULL;
}

void ui::CCVisualizerWidget::TextureOverlay::Update(const float* ndata) {
	// note: ndata only represents one channel
	static const int intFormat = GL_RGBA;

	#ifdef TEXTURE_DATATYPE_FLOAT
	static const int extFormat = GL_RED;
	static const int dataType  = GL_FLOAT;
	static const int bpp       = 1;
	#else
	static const int extFormat = GL_RGBA;
	static const int dataType  = GL_UNSIGNED_BYTE;
	static const int bpp       = 4;
	#endif

	float ndataMin =  std::numeric_limits<float>::max();
	float ndataMax = -std::numeric_limits<float>::max();

	if (ndata != NULL) {
		// find the extrema
		for (unsigned int i = 0; i < (sizex * sizey * stride); i += 1) {
			ndataMin = std::min(ndataMin, ndata[i]);
			ndataMax = std::max(ndataMax, ndata[i]);
		}
	}

	if (ndata != NULL) {
		// normalize
		#ifdef TEXTURE_DATATYPE_FLOAT
		for (unsigned int i = 0; i < (sizex * sizey * stride); i += bpp) {
			// note: size of {n}data must be a multiple of bpp
			data[i] = (ndata[i] - ndataMin) / (ndataMax - ndataMin);
		}
		#else
		for (unsigned int i = 0; i < (sizex * sizey * stride * bpp); i += bpp) {
			// note: size of {n}data must be a multiple of bpp
			data[i + 0] = (ndata[i / 4] < 0.0f)? 0: ((ndata[i / 4] / ndataMax) * 255);
			data[i + 1] =                                                          0;
			data[i + 2] = (ndata[i / 4] > 0.0f)? 0: ((ndata[i / 4] / ndataMin) * 255);
			data[i + 3] =                                                        255;
		}
		#endif
	} else {
		#ifdef TEXTURE_DATATYPE_FLOAT
		for (unsigned int i = 0; i < (sizex * sizey * stride); i += bpp) {
			data[i] = 0.0f;
		}
		#else
		for (unsigned int i = 0; i < (sizex * sizey * stride * bpp); i += bpp) {
			// note: size of data must be a multiple of bpp
			data[i + 0] =   0;
			data[i + 1] =   0;
			data[i + 2] =   0;
			data[i + 3] = 255;
		}
		#endif
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



ui::CCVisualizerWidget::VectorOverlay::VectorOverlay(
	unsigned int x,
	unsigned int y,
	unsigned int s,
	unsigned int dt,
	const vec3f* ndata
):
	Overlay(x, y, s, dt),
	data(new VertexArray())
{
	data->EnlargeArrays((sizex * sizey * stride) * 2, 0, VA_SIZE_C);
	Update(ndata);
}

ui::CCVisualizerWidget::VectorOverlay::~VectorOverlay() {
	delete data;
}

void ui::CCVisualizerWidget::VectorOverlay::Draw() {
	data->DrawArrayC(GL_LINES);
}

void ui::CCVisualizerWidget::VectorOverlay::Update(const vec3f* ndata) {
	static const unsigned char dirColors[5][4] = {
		{255,   0,   0, 255}, // red
		{  0, 255,   0, 255}, // green
		{  0,   0, 255, 255}, // blue
		{  0,   0,   0, 255}, // black
		{255, 255, 255, 255}, // white
	};

	if (ndata != NULL) {
		data->Initialize();

		const unsigned int numVectors = sizex * sizey * stride;
		// const unsigned int numVertices = numVectors * 2;
		// get the grid's downscale-factors (G2Hx == G2Hz)
		const unsigned int G2Hx = readMap->mapx / sizex;
		const unsigned int G2Hz = readMap->mapy / sizey;
		const unsigned int H2W  = readMap->SQUARE_SIZE;

		// if stride equals 4, then
		//    vectors 0 - 3 belong to cell 0
		//    vectors 4 - 7 belong to cell 1
		//    vectors 8 -11 belong to cell 2
		// so cell is given by i / stride
		switch (stride) {
			case 1: {
				for (unsigned int i = 0; i < numVectors; i += stride) {
					const vec3f& v = ndata[i];

					const unsigned int gx = i % sizex;
					const unsigned int gz = i / sizex;
					const float wx = (((gx * G2Hx) + (H2W >> 1)) * H2W);
					const float wz = (((gz * G2Hz) + (H2W >> 1)) * H2W);
					const float wy = ground->GetHeight(wx, wz);

					// NOTE: how to color these?
					data->AddVertexQC(vec3f(wx, wy,               wz), dirColors[4]);
					data->AddVertexQC(vec3f(wx, wy + v.sqLen3D(), wz), dirColors[0]);
				}
			} break;

			case 4: {
				for (unsigned int i = 0; i < numVectors; i += stride) {
					// note: size of ndata must be a multiple of stride
					const vec3f& vN = ndata[i + 0];
					const vec3f& vS = ndata[i + 1];
					const vec3f& vE = ndata[i + 2];
					const vec3f& vW = ndata[i + 3];

					const unsigned int j = i / stride;
					const unsigned int gx = j % sizex;
					const unsigned int gz = j / sizex;

					// get the world-space coordinates of the cell's center
					const float wx = (((gx * G2Hx) + (H2W >> 1)) * H2W);
					const float wz = (((gz * G2Hz) + (H2W >> 1)) * H2W);
					const float wy = ground->GetHeight(wx, wz);

					data->AddVertexQC(vec3f(wx,         wy + H2W, wz       ), dirColors[4]);
					data->AddVertexQC(vec3f(wx + vN.x,  wy + H2W, wz + vN.z), dirColors[0]);

					data->AddVertexQC(vec3f(wx,         wy + H2W, wz       ), dirColors[4]);
					data->AddVertexQC(vec3f(wx + vS.x,  wy + H2W, wz + vS.z), dirColors[2]);

					data->AddVertexQC(vec3f(wx,         wy + H2W, wz       ), dirColors[4]);
					data->AddVertexQC(vec3f(wx + vE.x,  wy + H2W, wz + vE.z), dirColors[3]);

					data->AddVertexQC(vec3f(wx,         wy + H2W, wz       ), dirColors[4]);
					data->AddVertexQC(vec3f(wx + vW.x,  wy + H2W, wz + vW.z), dirColors[1]);
				}
			} break;

			default: {
			} break;
		}
	}
}
