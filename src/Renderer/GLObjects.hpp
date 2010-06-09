#ifndef PFFG_GL_OBJECTS_HDR
#define PFFG_GL_OBJECTS_HDR

#include <vector>

#include "../Math/vec3fwd.hpp"
#include "../Math/vec4fwd.hpp"
#include "../Math/vec4.hpp"

namespace GL {
	struct Light {
	public:
		Light(int i): lightIdx(i), enabled(false) {}
		void Enable();
		void Disable();

		bool IsPositional() const { return (position.w > 0.0f); }

		const float* GetAmbientColor() const { return &ambientColor.x; }
		const float* GetDifuseColor() const { return &diffuseColor.x; }
		const float* GetSpecularColor() const { return &specularColor.x; }
	private:
		int lightIdx;
		bool enabled;

		vec4f ambientColor;
		vec4f diffuseColor;
		vec4f specularColor;
		vec4f position;
	};

	struct Texture {
	public:
		void Free();
		void Bind();
		void UnBind();

	private:
		unsigned int texID;
	};

	struct IBufferObject {
	public:
		void Free();
		void Bind();
		void UnBind();
	};
	struct VertexBufferObject: public IBufferObject {};
	struct PixelBufferObject: public IBufferObject {};
	struct FrameBufferObject: public IBufferObject {};
	struct RenderBufferObject: public IBufferObject {};
};

#endif
