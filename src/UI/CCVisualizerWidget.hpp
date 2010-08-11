#ifndef PFFG_CCWIDGET_HDR
#define PFFG_CCWIDGET_HDR

#include <map>
#include <vector>

#include "./UIWidget.hpp"
#include "../Math/vec3fwd.hpp"

class IPathModule;
class VertexArray;

namespace ui {
	struct CCVisualizerWidget: public IUIWidget {
	public:
		CCVisualizerWidget(): enabled(false) {
			currentTextureOverlay = NULL;
			currentVectorOverlay = NULL;

			visGroupIdx = 0;
			visGroupID = 0;
		}
		~CCVisualizerWidget();

		void Update(const vec3i&, const vec3i&);
		void KeyPressed(int);

	private:
		bool SetNextGroupID(const IPathModule*, bool, unsigned int);

		struct Overlay {
			public:
				Overlay(unsigned int x, unsigned int y, unsigned int s, unsigned int dt):
					enabled(true), sizex(x), sizey(y), stride(s), dataType(dt) {
				}

				virtual ~Overlay() {}

				virtual void Update(const float*) {}
				virtual void Update(const vec3f*) {} 
				virtual void SetEnabled(bool b) { enabled = b; }
				virtual bool IsEnabled() const { return enabled; }
				virtual unsigned int GetSizeX() const { return sizex; }
				virtual unsigned int GetSizeY() const { return sizey; }
				virtual unsigned int GetStride() const { return stride; }
				virtual unsigned int GetDataType() const { return dataType; }

			protected:
				bool enabled;

				unsigned int sizex;
				unsigned int sizey;
				unsigned int stride;

				unsigned int dataType;
		};

		struct TextureOverlay: public Overlay {
			public:
				TextureOverlay(unsigned int, unsigned int, unsigned int, unsigned int, const float*);
				~TextureOverlay();

				void Update(const float*);
				unsigned int GetID() const { return id; }
				#ifdef TEXTURE_DATATYPE_FLOAT
				float* GetData() { return data; }
				#else
				unsigned char* GetData() { return data; }
				#endif
			private:
				unsigned int id;

				#ifdef TEXTURE_DATATYPE_FLOAT
				float* data;
				#else
				unsigned char* data;
				#endif
		};

		struct VectorOverlay: public Overlay {
			public:
				VectorOverlay(unsigned int, unsigned int, unsigned int, unsigned int, const vec3f*);
				~VectorOverlay();

				void Update(const vec3f*);
				VertexArray* GetData() { return data; }
			private:
				VertexArray* data;
		};

		std::map<unsigned int, TextureOverlay*> textureOverlays;
		std::map<unsigned int, VectorOverlay*> vectorOverlays;
		std::vector<unsigned int> visGroupIDs;

		VectorOverlay* currentVectorOverlay;
		TextureOverlay* currentTextureOverlay;

		unsigned int visGroupIdx;
		unsigned int visGroupID;

		bool enabled;
	};
}

#endif
