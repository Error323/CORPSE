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



	// used by the custom SMF renderer
	struct VertexArray {
	public:
		VertexArray();

		void Initialize();
		void Enlarge();

		void AddVertex(float, float, float, float, float, float, float, float);
		void AddVertex(const vec3f&, const vec3f& n = YVECf, const vec3f& tc = NVECf);

		void Draw(int, const std::vector<int>&);
		
		vec3f GetVertex(int) const;
		vec3f GetNormal(int) const;

	private:
		std::vector<float> verts;
		std::vector<float> txcrs;
		std::vector<float> norms;
		unsigned int vDrawIdx;
		unsigned int tDrawIdx;
		unsigned int nDrawIdx;
	};

	struct VertexArrayManager {
	public:
		VertexArrayManager();
		~VertexArrayManager();

		VertexArray* GetVertexArray();

	private:
		VertexArray* va0;	// primary buffer
		VertexArray* va1;	// secondary buffer
		VertexArray* va;	// currently active buffer
	};
};

#endif
