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
	/*
	virtual void Update(const Camera*) = 0;

	virtual void IncreaseDetail() = 0;
	virtual void DecreaseDetail() = 0;

	// everything that deals with drawing extra textures on top
	void DisableExtraTexture();
	void SetHeightTexture();
	void SetMetalTexture(unsigned char* tex, float* extractMap, unsigned char* pal, bool highRes);
	void SetPathMapTexture();
	void ToggleLosTexture();
	void ToggleRadarAndJammer();

	bool UpdateExtraTexture();
	bool DrawExtraTex() const { return (drawMode != drawNormal); }
	*/

	/*
	bool updateFov;
	bool drawRadarAndJammer;
	bool drawLineOfSight;
	*/
	bool wireframe;

	//// float LODScaleReflection;
	//// float LODScaleRefraction;
	//// float LODScaleUnitReflection;

	/*GLu*/ unsigned int infoTex;
	Shader::IProgramObject* shaderProObj;

	/*
	unsigned char* infoTexMem;
	bool highResInfoTex;
	bool highResInfoTexWanted;

	const unsigned char* extraTex;
	const unsigned char* extraTexPal;
	float* extractDepthMap;
	int updateTextureState;

	float infoTexAlpha;

	int jamColor[3];
	int losColor[3];
	int radarColor[3];
	int alwaysColor[3];
	static const int losColorScale = 10000;

	bool highResLosTex;
	// bool smoothLosTex;

	CHeightLinePalette* heightLinePal;
	*/

	/*
	enum DrawMode {
		drawNormal,
		drawLos,
		drawMetal,
		drawHeight,
		drawPath
	};
	DrawMode drawMode;
	*/

/*
protected:
	virtual void SetDrawMode(DrawMode dm);
*/
};

#endif
