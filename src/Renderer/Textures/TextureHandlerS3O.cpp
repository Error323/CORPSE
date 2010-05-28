#include <GL/gl.h>
#include <cassert>

#include "./TextureHandlerS3O.hpp"
#include "./BitMap.hpp"
#include "../Models/ModelReaderBase.hpp"
#include "../../System/EngineAux.hpp"
#include "../../System/FileHandler.hpp"
#include "../../System/LuaParser.hpp"
#include "../../System/Logger.hpp"

CTextureHandlerS3O* textureHandlerS3O = 0x0;

CTextureHandlerS3O::CTextureHandlerS3O() {
	// distinction between 3DO and S3O textures
	// is made via model->textureType, which is
	// interpreted as S3O only if greater than 0
	// (hence the dummy insertion)
	textures.push_back(TexS3O());
}

CTextureHandlerS3O::~CTextureHandlerS3O() {
	while (textures.size() > 1) {
		glDeleteTextures(1, &(textures[textures.size() - 1].tex1));
		glDeleteTextures(1, &(textures[textures.size() - 1].tex2));
		textures.pop_back();
	}
}



void CTextureHandlerS3O::Load(ModelBase* model) {
	std::string tex1 = model->tex1;
	std::string tex2 = model->tex2;
	std::string totalName = tex1 + tex2;

	LOG << "[CTextureHandlerS3O::LoadTextures]\n";

	if (textureNames.find(totalName) != textureNames.end()) {
		LOG << "\tfound cached texture-set ";
		LOG << totalName << " at index ";
		LOG << textureNames[totalName] << "\n";

		model->textureType = textureNames[totalName];
		return;
	}

	std::string texDir = LUA->GetRoot()->GetTblVal("general")->GetStrVal("texturesDir", "data/textures/") + "units/";

	CBitMap texture1;
	CBitMap texture2;

	if (!texture1.Load(texDir + tex1)) {
		LOG << "\tcould not load texture #1 ";
		LOG << "(\"" << tex1 << "\") for model ";
		LOG << model->name << "\n";

		model->textureType = -1;
		assert(false);
		return;
	}

	// always >= 1 due to dummy first element
	const int idx = textures.size();

	TexS3O tex;
	tex.idx = idx;
	tex.tex1 = texture1.CreateTexture(true);
	tex.tex1SizeX = texture1.xsize;
	tex.tex1SizeY = texture1.ysize;
	tex.tex2 = 0;
	tex.tex2SizeX = 0;
	tex.tex2SizeY = 0;

	// no error checking here, other code relies on empty
	// texture being generated if it couldn't be loaded
	if (!texture2.Load(texDir + tex2)) {
		LOG << "\tcould not load texture #2 ";
		LOG << "(\"" << tex2 << "\") for model ";
		LOG << model->name << "\n";

		texture2.Alloc(1, 1);
		texture2.mem[3] = 255;
		// assert(false);
		// return;
	}

	tex.tex2 = texture2.CreateTexture(true);
	tex.tex2SizeX = texture2.xsize;
	tex.tex2SizeY = texture2.ysize;

	textures.push_back(tex);
	textureNames[totalName] = idx;

	LOG << "\tloaded textures (key: " << totalName << ") for";
	LOG << " model " << model->name << " (idx: " << idx << ")\n";

	// must be greater than 0 for S3O's
	model->textureType = idx;
}

void CTextureHandlerS3O::Bind(const ModelBase* m) const {
	/*
	if (shadowHandler->inShadowPass) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, textures[m->textureType].tex2);
	} else {
	*/
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textures[m->textureType].tex1);

		if (true /*unitDrawer->advShading*/) {
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, textures[m->textureType].tex2);
		}
	}
}

void CTextureHandlerS3O::UnBind() const {
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
}
