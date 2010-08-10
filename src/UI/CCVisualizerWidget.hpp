#ifndef PFFG_CCWIDGET_HDR
#define PFFG_CCWIDGET_HDR

#include <map>
#include <vector>

#include "./UIWidget.hpp"
#include "../Math/vec3fwd.hpp"

class VertexArray;

namespace ui {
	struct CCVisualizerWidget: public IUIWidget {
	public:
		CCVisualizerWidget(): enabled(false) {
			texGroupIdx = 0;
			texGroupID = 0;
		}
		~CCVisualizerWidget();

		void Update(const vec3i&, const vec3i&);
		void KeyPressed(int);

	private:
		struct Texture {
		public:
			Texture(unsigned int, unsigned int, unsigned int, const float*);
			~Texture();

			void Update(const float*);
			void ToggleEnabled() { enabled = !enabled; }
			bool IsEnabled() const { return enabled; }
			unsigned int GetID() const { return id; }
			unsigned int GetSizeX() const { return sizex; }
			unsigned int GetSizeY() const { return sizey; }

		private:
			bool enabled;

			unsigned int id;
			unsigned int sizex;
			unsigned int sizey;
			unsigned int stride;

			#ifdef TEXTURE_DATATYPE_FLOAT
			float* data;
			#else
			unsigned char* data;
			#endif
		};

		std::map<unsigned int, VertexArray*> vectorOverlays;
		std::map<unsigned int, Texture*> textureOverlays;
		std::vector<unsigned int> textureGroupIDs;

		unsigned int texGroupIdx;
		unsigned int texGroupID;

		bool enabled;
	};
}

#endif
