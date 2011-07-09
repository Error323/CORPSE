#include <FTGL/ftgl.h>
#include <FTGL/FTGLTextureFont.h>

#include "./FontManager.hpp"
#include "../System/EngineAux.hpp"
#include "../System/Logger.hpp"

ui::IFontManager* ui::IFontManager::GetInstance() {
	static IFontManager* fm = NULL;
	static unsigned int depth = 0;

	if (fm == NULL) {
		PFFG_ASSERT(depth == 0);

		depth += 1;
		fm = new FTGLFontManager();
		depth -= 1;
	}

	return fm;
}

void ui::IFontManager::FreeInstance(IFontManager* fm) {
	delete fm;
}



ui::FTGLFontManager::~FTGLFontManager() {
	for (FontMapIt it = fonts.begin(); it != fonts.end(); ++it) {
		delete it->second;
	}

	fonts.clear();
}

FTFont* ui::FTGLFontManager::GetFont(const std::string& fontName, int fontSize) {
	FontMapIt it = fonts.find(fontName);

	if (it != fonts.end()) {
		return it->second;
	}

        FTTextureFont* font = new FTTextureFont(fontName.c_str());

        if (!font->FaceSize(fontSize)) {
                LOG << "[FTGLFontManager::GetFont] failed to set size " << fontSize << " for font " << fontName;
                delete font;
                return NULL;
        }

	fonts[fontName] = font;
	return font;
}
