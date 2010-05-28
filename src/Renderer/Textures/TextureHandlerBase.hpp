#ifndef TEXTUREHANDLERBASE_HDR
#define TEXTUREHANDLERBASE_HDR

struct ModelBase;
class CTextureHandlerBase {
	public:
		virtual ~CTextureHandlerBase() {}
		virtual void Load(ModelBase*) = 0;
		virtual void Bind(const ModelBase*) const = 0;
		virtual void UnBind() const = 0;
};

#endif
