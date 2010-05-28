#ifndef PFFG_MAPINFO_HDR
#define PFFG_MAPINFO_HDR

#include <string>
#include <vector>

#include "../Math/vec3fwd.hpp"
#include "../Math/vec4fwd.hpp"
#include "../Math/vec3.hpp"
#include "../Math/vec4.hpp"


class CMapInfo {
public:
	static const CMapInfo* GetInstance(const std::string& = "");
	static void FreeInstance(const CMapInfo*);

	/** Global settings, ie. from "MAP" section. */
	struct map_t {
		std::string name;      ///< The filename as passed to the constructor.
		std::string wantedScript;
		std::string humanName; ///< "MAP\\Description"
		std::string author;
		float hardness;        ///< "MAP\\MapHardness"
		bool  notDeformable;
		/** Stores the gravity as a negative number in units/frame^2
			(NOT positive units/second^2 as in the mapfile) */
		float gravity;
		float tidalStrength;
		float maxMetal;
		float extractorRadius; ///< extraction radius for mines
		bool  voidWater;
	} map;

	/** GUI settings (used by CGuiHandler) */
	struct gui_t {
		bool autoShowMetal;
	} gui;

	/** settings read from "MAP\ATMOSPHERE" section */
	struct atmosphere_t {
		float  cloudDensity;
		float  fogStart;
		vec4f fogColor;
		vec3f skyColor;
		vec3f sunColor;
		vec3f cloudColor;
		float  minWind;
		float  maxWind;
		std::string skyBox;
	} atmosphere;

	/** settings read from "MAP\LIGHT" section */
	struct light_t {
		vec4f sunDir;
		vec4f groundAmbientColor;
		vec4f groundDiffuseColor;
		vec4f groundSpecularColor;
		float groundShadowDensity;
		vec4f unitAmbientColor;
		vec4f unitSunColor;
		float unitShadowDensity;
		// vec3f specularSunColor;
	} light;

	/** settings read from "MAP\WATER" section
		prefix their name with "Water" to get the TDF variable */
	struct water_t {
		float repeatX; ///< (calculated default is in CBaseWater)
		float repeatY; ///< (calculated default is in CBaseWater)
		float damage;
		vec3f absorb;
		vec3f baseColor;
		vec3f minColor;
		vec3f surfaceColor;
		float surfaceAlpha;
		vec4f planeColor;
		vec3f diffuseColor;
		vec3f specularColor;
		float ambientFactor;
		float diffuseFactor;
		float specularFactor;
		float specularPower;
		float fresnelMin;
		float fresnelMax;
		float fresnelPower;
		float reflDistortion;
		float blurBase;
		float blurExponent;
		float perlinStartFreq;
		float perlinLacunarity;
		float perlinAmplitude;
		bool  shoreWaves;
		unsigned char numTiles;
		std::string texture;
		std::string foamTexture;
		std::string normalTexture;
		std::vector<std::string> causticTextures;
	} water;
	bool hasWaterPlane; ///< true if "MAP\WATER\WaterPlaneColor" is set

	/** SMF specific settings */
	struct smf_t {
		std::string detailTexName; ///< "MAP\DetailTex"
		float minHeight;
		bool  minHeightOverride;
		float maxHeight;
		bool  maxHeightOverride;
	} smf;

	/** SM3 specific settings
		This is NOT complete, SM3 stores so much in the map settings
		that it really isn't a good idea to put them here. */
	struct sm3_t {
		std::string minimap; ///< "MAP\minimap"
	} sm3;

	/** Terrain type, there can be 256 of these:
		"MAP\TerrainType0" up to "MAP\TerrainType255" */
	struct TerrainType {
		std::string name;
		float hardness;
		float tankSpeed;   ///< "TankMoveSpeed"
		float kbotSpeed;   ///< "KbotMoveSpeed"
		float hoverSpeed;  ///< "HoverMoveSpeed"
		float shipSpeed;   ///< "ShipMoveSpeed"
		bool receiveTracks;
	};
	TerrainType terrainTypes[256];

	bool GetStartPos(int team, vec3f& pos) const;

private:
	CMapInfo(const std::string& mapName);

	std::vector<bool> havePos;
	std::vector<vec3f> startPos;

	void ReadGlobal();
	void ReadGui();
	void ReadAtmosphere();
	void ReadLight();
	void ReadWater();
	void ReadSMf();
	void ReadTerrainTypes();
	void ReadStartPos();
};

#define mapInfo (CMapInfo::GetInstance())

#endif
