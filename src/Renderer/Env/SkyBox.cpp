#include <GL/gl.h>

#include <vector>
#include <cassert>
#include <cstdlib>
#include <sstream>

#include "./SkyBox.hpp"

#include "../GL.hpp"
#include "../Camera.hpp"
#include "../Textures/BitMap.hpp"
#include "../../Math/vec3.hpp"
#include "../../UI/Window.hpp"
#include "../../System/EngineAux.hpp"
#include "../../System/FileHandler.hpp"
#include "../../System/LuaParser.hpp"

CSkyBox::CSkyBox() {
	std::list<int> skyBoxKeys;
	std::vector<std::string> skyBoxNames;

	const LuaTable* rootTable = LUA->GetRoot();
	const LuaTable* generalTable = rootTable->GetTblVal("general");
	const LuaTable* skyTable = rootTable->GetTblVal("sky");

	const std::string& skyBoxPath = generalTable->GetStrVal("texturesDir", "data/textures") + "skyboxes/";

	skyTable->GetIntStrKeys(&skyBoxKeys);

	for (std::list<int>::const_iterator it = skyBoxKeys.begin(); it != skyBoxKeys.end(); ++it) {
		const std::string& skyBoxName = skyTable->GetStrVal(*it, "-");

		CFileHandler fh(skyBoxPath + skyBoxName);

		if (fh.FileExists()) {
			skyBoxNames.push_back(skyBoxPath + skyBoxName);
		}
	}

	assert(!skyBoxNames.empty());
	CBitMap skyBoxBM;

	if (!skyBoxBM.Load( skyBoxNames[random() % skyBoxNames.size()] )) {
		textureID = 0;
	} else {
		textureID = skyBoxBM.CreateTexture(0);
	}
}

CSkyBox::~CSkyBox(void) {
	glDeleteTextures(1, &textureID);
}

void CSkyBox::Draw(const Camera* cam) {
	// glTranslatef(cam->pos.x, cam->pos.y, cam->pos.z);
	// glCallList(displayListID);

	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_FOG);
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_BLEND);

		glDepthMask(GL_FALSE);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);

		glEnable(GL_TEXTURE_CUBE_MAP_ARB);
		glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, textureID);

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		const uint vpsx = gWindow->GetViewPort().size.x;
		const uint vpsy = gWindow->GetViewPort().size.y;
		const vec3f v1 = cam->GetPixelDir(   0,    0), tl = cam->pos + v1 * (0.125f + 100);
		const vec3f v2 = cam->GetPixelDir(vpsx,    0), tr = cam->pos + v2 * (0.125f + 100);
		const vec3f v3 = cam->GetPixelDir(vpsx, vpsy), br = cam->pos + v3 * (0.125f + 100);
		const vec3f v4 = cam->GetPixelDir(   0, vpsy), bl = cam->pos + v4 * (0.125f + 100);

		glPushMatrix();
			glBegin(GL_QUADS);
				glTexCoord3f(-v1.x, -v1.y, -v1.z);
				// glNormal3f(0, 1, 0);
				glVertex3f(tl.x, tl.y, tl.z);

				glTexCoord3f(-v2.x, -v2.y, -v2.z);
				// glNormal3f(1, 1, 0);
				glVertex3f(tr.x, tr.y, tr.z);

				glTexCoord3f(-v3.x, -v3.y, -v3.z);
				// glNormal3f(0, 1, 1);
				glVertex3f(br.x, br.y, br.z);

				glTexCoord3f(-v4.x, -v4.y, -v4.z);
				// glNormal3f(1, 1, 1);
				glVertex3f(bl.x, bl.y, bl.z);
			glEnd();
		glPopMatrix();

		glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, 0);
		glDisable(GL_TEXTURE_CUBE_MAP_ARB);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		/*
		if (mapInfo->atmosphere.fogStart < 0.99f) {
			glFogfv(GL_FOG_COLOR, mapInfo->atmosphere.fogColor);
			glFogi(GL_FOG_MODE, GL_LINEAR);
			glFogf(GL_FOG_START, gu->viewRange * mapInfo->atmosphere.fogStart);
			glFogf(GL_FOG_END, gu->viewRange);
			glFogf(GL_FOG_DENSITY, 1.0f);
			glEnable(GL_FOG);
		}
		*/
	glPopAttrib();
}
