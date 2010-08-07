#ifndef PFFG_BASEGROUNDDRAWER_HDR
#define PFFG_BASEGROUNDDRAWER_HDR

#include "../Renderer/Shaders/Shader.hpp"

struct Camera;

//// class CHeightLinePalette;
class CBaseGroundDrawer {
public:
	CBaseGroundDrawer(void);
	virtual ~CBaseGroundDrawer(void);

	virtual void Draw(const Camera*, bool) = 0;
	virtual void SetOverlayTexture(unsigned int texID) { overlayTexID = texID; }

	Shader::IProgramObject* shaderProObj;

protected:
	bool wireframe;
	unsigned int overlayTexID;
};

#endif
