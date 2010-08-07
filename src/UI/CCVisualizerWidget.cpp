#include <SDL/SDL.h>
#include <GL/gl.h>

#include "./CCVisualizerWidget.hpp"
#include "../Map/BaseGroundDrawer.hpp"
#include "../Map/ReadMap.hpp"
#include "../Math/vec3.hpp"
#include "../Renderer/RenderThread.hpp"
#include "../Sim/SimThread.hpp"
#include "../Path/CC/CCPathModule.hpp"

ui::CCVisualizerWidget::~CCVisualizerWidget() {
	for (std::map<unsigned int, Texture*>::iterator it = textures.begin(); it != textures.end(); ++it) {
		delete it->second;
	}

	textures.clear();
}

void ui::CCVisualizerWidget::KeyPressed(int key) {
	if (key == SDLK_v) {
		enabled = !enabled;
	}

	if (enabled) {
		const IPathModule* m = simThread->GetPathModule();

		Texture* heightTex = textures[PathModule::DATATYPE_HEIGHT];
		Texture* potentialTex = textures[PathModule::DATATYPE_POTENTIAL];

		if (key == SDLK_h) {
			if (heightTex == NULL) {
				// create the height-texture
				const unsigned int xsize = m->GetScalarDataArraySizeX(PathModule::DATATYPE_HEIGHT);
				const unsigned int zsize = m->GetScalarDataArraySizeZ(PathModule::DATATYPE_HEIGHT);
				const float* data = m->GetScalarDataArray(PathModule::DATATYPE_HEIGHT, -1);

				heightTex = new Texture(xsize, zsize, data);
				textures[PathModule::DATATYPE_HEIGHT] = heightTex;

				readMap->GetGroundDrawer()->SetOverlayTexture(heightTex->GetID());
			} else {
				heightTex->ToggleEnabled();

				if (!heightTex->IsEnabled()) {
					readMap->GetGroundDrawer()->SetOverlayTexture(0);
				} else {
					readMap->GetGroundDrawer()->SetOverlayTexture(heightTex->GetID());
				}
			}
		}

		if (key == SDLK_p) {
			if (potentialTex == NULL) {
				// create the potential-texture
				const unsigned int xsize = m->GetScalarDataArraySizeX(PathModule::DATATYPE_POTENTIAL);
				const unsigned int zsize = m->GetScalarDataArraySizeZ(PathModule::DATATYPE_POTENTIAL);
				const float* data = m->GetScalarDataArray(PathModule::DATATYPE_POTENTIAL, texGroupIdx);

				potentialTex = new Texture(xsize, zsize, data);
				textures[PathModule::DATATYPE_POTENTIAL] = potentialTex;

				readMap->GetGroundDrawer()->SetOverlayTexture(potentialTex->GetID());
			} else {
				potentialTex->ToggleEnabled();

				if (!potentialTex->IsEnabled()) {
					readMap->GetGroundDrawer()->SetOverlayTexture(0);
				} else {
					// cycle to the next group
					const unsigned int numGroupIDs = m->GetNumGroupIDs();

					if (numGroupIDs > 0) {
						textureGroupIDs.resize(numGroupIDs);
						m->GetGroupIDs(&textureGroupIDs[0], numGroupIDs);

						texGroupIdx = (texGroupIdx + 1) % numGroupIDs;
						texGroupID = textureGroupIDs[texGroupIdx];

						potentialTex->Update(m->GetScalarDataArray(PathModule::DATATYPE_POTENTIAL, texGroupID));
						readMap->GetGroundDrawer()->SetOverlayTexture(potentialTex->GetID());
					}
				}
			}
		}
	}
}

void ui::CCVisualizerWidget::Update(const vec3i&, const vec3i&) {
	static unsigned int simFrame = simThread->GetFrame();

	if (enabled) {
		Texture* potentialTex = textures[PathModule::DATATYPE_POTENTIAL];

		if (potentialTex != NULL && potentialTex->IsEnabled()) {
			if (simThread->GetFrame() != simFrame) {
				simFrame = simThread->GetFrame();

				// update the texture data
				// if the group corresponding to <texGroupID> no longer exists,
				// this causes the texture to be filled with default values (0)
				const IPathModule* m = simThread->GetPathModule();
				const float* d = m->GetScalarDataArray(PathModule::DATATYPE_POTENTIAL, texGroupID);

				potentialTex->Update(d);
			}
		}
	}
}



ui::CCVisualizerWidget::Texture::Texture(unsigned int x, unsigned int y, const float* ndata):
	enabled(true),
	id(0),
	sizex(x),
	sizey(y),
	#ifdef TEXTURE_DATATYPE_FLOAT
	data(new float[sizex * sizey])
	#else
	data(new unsigned char[sizex * sizey * 4])
	#endif
{
	Update(ndata);
}

ui::CCVisualizerWidget::Texture::~Texture() {
	glDeleteTextures(1, &id); id = 0;
	delete[] data; data = NULL;
}

void ui::CCVisualizerWidget::Texture::Update(const float* ndata) {
	// note: ndata only represents one channel
	static const int intFormat = GL_RGBA;

	#ifdef TEXTURE_DATATYPE_FLOAT
	static const int extFormat = GL_RED;
	static const int dataType  = GL_FLOAT;
	#else
	static const int extFormat = GL_RGBA;
	static const int dataType  = GL_UNSIGNED_BYTE;
	#endif

	float ndataMin =  1e9f;
	float ndataMax = -1e9f;

	if (ndata != NULL) {
		// find the extrema
		for (unsigned int i = 0; i < (sizex * sizey); i++) {
			ndataMin = std::min(ndataMin, ndata[i]);
			ndataMax = std::max(ndataMax, ndata[i]);
		}
	}

	if (ndata != NULL) {
		// normalize
		#ifdef TEXTURE_DATATYPE_FLOAT
		for (unsigned int i = 0; i < (sizex * sizey); i += 1) {
			data[i] = (ndata[i] - ndataMin) / (ndataMax - ndataMin);
		}
		#else
		for (unsigned int i = 0; i < (sizex * sizey * 4); i += 4) {
			data[i + 0] = (ndata[i / 4] < 0.0f)? 0: ((ndata[i / 4] / ndataMax) * 255);
			data[i + 1] =                                                          0;
			data[i + 2] = (ndata[i / 4] > 0.0f)? 0: ((ndata[i / 4] / ndataMin) * 255);
			data[i + 3] =                                                        255;
		}
		#endif
	} else {
		#ifdef TEXTURE_DATATYPE_FLOAT
		for (unsigned int i = 0; i < (sizex * sizey); i += 1) {
			data[i] = 0.0f;
		}
		#else
		for (unsigned int i = 0; i < (sizex * sizey * 4); i += 4) {
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
