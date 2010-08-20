#ifndef PFFG_CCWIDGET_HDR
#define PFFG_CCWIDGET_HDR

#include <map>
#include <vector>

#include "./UIWidget.hpp"
#include "../Math/vec3fwd.hpp"
#include "../Path/IPathModule.hpp"

struct DataTypeInfo;
class VertexArray;

namespace ui {
	struct CCVisualizerWidget: public IUIWidget {
	public:
		CCVisualizerWidget(): enabled(false) {
			currentTextureOverlay = NULL;
			currentVectorOverlay = NULL;

			mModule = NULL;

			texVisGroupIdx = 0; texVisGroupID = 0;
			vecVisGroupIdx = 0; vecVisGroupID = 0;
		}
		~CCVisualizerWidget();

		void Update(const vec3i&, const vec3i&);
		void KeyPressed(int);

	private:
		bool SetNextVisGroupID(bool);

		struct Overlay {
			public:
				Overlay(const IPathModule::DataTypeInfo*);
				virtual ~Overlay() {}

				virtual bool IsGlobal() const { return global; }
				virtual void SetEnabled(bool b) { enabled = b; }
				virtual bool IsEnabled() const { return enabled; }
				virtual unsigned int GetSizeX() const { return sizex; }
				virtual unsigned int GetSizeY() const { return sizey; }
				virtual unsigned int GetStride() const { return stride; }
				virtual unsigned int GetDataType() const { return dataType; }

			protected:
				bool global;
				bool enabled;

				unsigned int sizex;
				unsigned int sizey;
				unsigned int stride;

				unsigned int dataType;
		};

		struct TextureOverlay: public Overlay {
			public:
				TextureOverlay(const IPathModule::DataTypeInfo*);
				~TextureOverlay();

				void Update(const float*);
				unsigned int GetID() const { return id; }
			private:
				unsigned int id;
				unsigned char* data;
		};

		struct VectorOverlay: public Overlay {
			public:
				VectorOverlay(const IPathModule::DataTypeInfo*);
				~VectorOverlay();

				void Update(const vec3f*);
				void Draw();
			private:
				VertexArray* data;
		};

		std::map<unsigned int, TextureOverlay*> textureOverlays;
		std::map<unsigned int, VectorOverlay*> vectorOverlays;
		std::vector<unsigned int> visGroupIDs;

		VectorOverlay* currentVectorOverlay;
		TextureOverlay* currentTextureOverlay;

		const IPathModule* mModule;

		unsigned int texVisGroupIdx, texVisGroupID;
		unsigned int vecVisGroupIdx, vecVisGroupID;

		bool enabled;
	};
}

#endif
