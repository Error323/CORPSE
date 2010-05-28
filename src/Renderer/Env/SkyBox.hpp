#ifndef PFFG_SKYBOX_HDR
#define PFFG_SKYBOX_HDR

#include <string>

struct Camera;

class CSkyBox /*: public CBaseSky*/ {
	public:
		CSkyBox();
		~CSkyBox();

		void Draw(const Camera*);

	private:
		unsigned int textureID;
		unsigned int displayListID;
	};

#endif
