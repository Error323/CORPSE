#ifndef PFFG_SHADOWHANDLER_HDR
#define PFFG_SHADOWHANDLER_HDR

#include "../../Math/mat44fwd.hpp"
#include "../../Math/mat44.hpp"

struct Camera;

class CShadowHandler {
public:
	CShadowHandler();

	void GenDepthTextureFBO();
	void DrawDepthTexture();

	unsigned int GetDepthTextureID() const { return texID; }

	void GenShadowProjectionMatrix(Camera*);
	const float* GetShadowProjectionMatrix() { return (&shadowProjectionMatrix.m[0]); }

	void BindDepthTextureFBO();
	void UnBindDepthTextureFBO();

	static CShadowHandler* GetInstance();

private:
	// multiple light-sources would need one
	// FBO each, since only one depth-texture
	// attachment per FBO is supported
	unsigned int fboID;
	unsigned int texID;

	unsigned int SIZE_MULT_X;
	unsigned int SIZE_MULT_Y;

	// the sun-camera stores this too, but
	// without the bias-matrix component
	mat44f shadowProjectionMatrix;

	bool basicShadows;
	bool enabled;
};

#define shadowHandler (CShadowHandler::GetInstance())

#endif
