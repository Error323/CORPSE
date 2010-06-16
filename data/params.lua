params = {
	["general"] = {
		-- note: paths are wrt. the CWD
		fontsDir        = "./data/fonts/",
		mapsDir         = "./data/maps/",
		modelsDir       = "./data/models/",
		shadersDir      = "./data/shaders/",
		texturesDir     = "./data/textures/",
		logDir          = "./data/logs/",

		-- only meaningful in FPS mode
		mouseLook       =  0,

		lineSmoothing   =  1,
		pointSmoothing  =  1,
	},

	["ui"] = {
		fontName        = "Vera.ttf",
		fontSize        = 72,
	},

	["server"] = {
		simFrameRate    = 20,
		simRateMult     =  1,
	},

	["window"] = {
		xsize           = 1024,
		ysize           =  768,

		useFSAA         =  1,
		FSAALevel       =  4,
		fullScreen      =  0,
		dualScreen      =  0,
		bitsPerPixel    = 32,
		depthBufferBits = 24,
	},
	["viewport"] = {
		xsize = 1024,
		ysize =  768,
	},

	["input"] = {
		inputRate = 100,

		-- multiply by 100, because the Lua
		-- library package is configured for
		-- integer math only
		keySens   = 0.5 * 100.0,
		mouseSens = 0.5 * 100.0,

		["keybindings"] = {
		}
	},

	["camera"] = {
		moveMode = 2, -- overhead
		projMode = 0, -- perspective

		-- set the vertical FOV to 22.5 degrees
		-- up / down; horizontal FOV is derived
		-- from this
		vFOV = 45.0,

		zNearDist =    16.0,
		zFarDist  = 16384.0,

		pos = {1024.0, 1536.0, 2048.0},
		vrp = {1024.0,    0.0,    0.0},
	},

	["map"] = {
		smf = "SmallDivide.smf",
		smt = "SmallDivide.smt",
		-- smf = "TabulaV2.smf",
		-- smt = "TabulaV2.smt",

		groundAmbientColor  = {0.2 * 100.0, 0.2 * 100.0, 0.2 * 100.0, 1.0 * 100.0},
		groundDiffuseColor  = {1.0 * 100.0, 1.0 * 100.0, 1.0 * 100.0, 1.0 * 100.0},
		groundSpecularColor = {0.8 * 100.0, 0.8 * 100.0, 0.8 * 100.0, 1.0 * 100.0},

		-- vector from origin to light-source; fourth
		-- component indicates whether light should be
		-- directional (0) or positional (1)
		-- this is used to derive a "position" for the
		-- sun camera, otherwise passed to OGL directly
		-- the sun's light direction is the opposite of
		-- the vector to its position!
		sunDir = {5.0, 1.0, 5.0, 0.0 * 100.0},
		sunType = 0.0 * 100.0,

		viewRadius = 128.0,

		vShader = "SMFVP.glsl",
		fShader = "SMFFP.glsl",
	},

	["sky"] = {
		[0] = "altored.dds",
		[1] = "cleardesert.dds",
		[2] = "cloudysunset.dds",
		[3] = "eveningsky.dds",
		[4] = "nebulus.dds",
	},

	["objectdefs"] = {
		core_goliath = {
			mdl                  = "core_goliath.s3o",
			maxForwardSpeed      =  50.0,
			maxTurningRate       =  20.0,
			maxAccelerationRate  =   1.0,
			maxDeccelerationRate =   1.0,
		},
	},

	["objects"] = {
		numObjectGridCells = {64, 1, 64},

		[1] = {
			def = "core_goliath",
			pos = {200.0, 0.0, 200.0},
			dir = {  0.0, 0.0,   1.0},
		},
		[2] = {
			def = "core_goliath",
			pos = {3800.0, 0.0, 3800.0},
			dir = {   0.0, 0.0,   -1.0},
		},
	},

	["models"] = {
		["core_goliath.s3o"] = {
			vShader = "S3OVP.glsl",
			fShader = "S3OFP.glsl",
		},
	},


	--[[
		"arm_flea.s3o",
		name = "core_goliath.s3o",
		name = "core_reaper.s3o",
		name = "core_leveler.s3o",
		name = "core_raider.s3o",
		name = "core_slasher.s3o",
		name = "core_instigator.s3o",
		name = "core_banisher.s3o",
		name = "core_construction_vehicle.s3o",
		name = "core_mobile_artillery.s3o",
		name = "core_tremor.s3o",
		name = "core_radar_vehicle.s3o",
		name = "core_wolverine.s3o",
		name = "core_crock.s3o",
		name = "core_garpike.s3o",
		name = "core_weasel.s3o",
		name = "core_diplomat.s3o",
		name = "core_flak_vehicle.s3o", "corsent"
		name = "UltraAssault.s3o",
	--]]
}
