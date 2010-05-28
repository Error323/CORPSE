#ifndef TEXTUREHANDLERS3O_HDR
#define TEXTUREHANDLERS3O_HDR

#include <string>
#include <map>
#include <vector>

#include "./TextureHandlerBase.hpp"

struct ModelBase;
class CFileHandler;

class CTextureHandlerS3O: public CTextureHandlerBase {
	public:
		CTextureHandlerS3O();
		~CTextureHandlerS3O();

		struct TexS3O {
			TexS3O(): idx(0), tex1(0), tex2(0) {
			}

			unsigned int idx;
			unsigned int tex1;
			unsigned int tex1SizeX;
			unsigned int tex1SizeY;
			unsigned int tex2;
			unsigned int tex2SizeX;
			unsigned int tex2SizeY;
		};

		void Load(ModelBase* model);
		void Bind(const ModelBase* model) const;
		void UnBind() const;

		const TexS3O* GetTextures(int num) const {
			if ((num < 0) || (num >= int(textures.size()))) {
				return NULL;
			}
			return &textures[num];
		}

	private:
		std::map<std::string, int> textureNames;
		std::vector<TexS3O> textures;
};

extern CTextureHandlerS3O* textureHandlerS3O;

#endif
