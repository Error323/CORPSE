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
		fontSize        = 60,
	},

	["server"] = {
		simFrameRate    = 25,
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

		pos = {2048.0, 4096.0, 4096.0},
		vrp = {2048.0,    0.0, 2048.0},
	},

	["map"] = {
		smf = "SmallDivide.smf",
		smt = "SmallDivide.smt",
		-- smf = "TabulaV2.smf",
		-- smt = "TabulaV2.smt",

		flatten = 1,

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

	["teams"] = {
		[0] = {color = {1.0, 1.0, 1.0, 1.0}},   -- white
		[1] = {color = {1.0, 0.0, 0.0, 1.0}},   -- red
		[2] = {color = {0.0, 1.0, 0.0, 1.0}},   -- green
		[3] = {color = {0.0, 0.0, 1.0, 1.0}},   -- blue
		[4] = {color = {1.0, 1.0, 0.0, 1.0}},   -- yellow
		[5] = {color = {0.0, 1.0, 1.0, 1.0}},   -- teal
		[6] = {color = {1.0, 0.0, 1.0, 1.0}},   -- purple
		[7] = {color = {0.0, 0.0, 0.0, 1.0}},   -- black
	},

	["objectdefs"] = {
		core_goliath = {
			mdl                  = "core_goliath.s3o",

			--  [[
			maxForwardSpeed      = 100.0,
			maxTurningRate       =  45.0,
			maxAccelerationRate  =   5.0,
			maxDeccelerationRate =   5.0,
			--  ]]

			--[[
			maxForwardSpeed      = 100.0, -- units per second^1
			maxTurningRate       = 180.0, -- degrees per second^1
			maxAccelerationRate  =  50.0, -- units per second^2
			maxDeccelerationRate =  25.0, -- units per second^2
			--]]
		},
	},

	["objects"] = {
		numObjectGridCells = {64, 1, 64},

	},

	["models"] = {
		["core_goliath.s3o"] = {
			scale   = 1.0,
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

	["pathmodule"] = {
		["cc"] = {
			alpha   =  1.0,  -- speed weight
			beta    =  0.0,  -- time weight
			gamma   =  3.0,  -- discomfort weight

			rho_bar = 0.20,  -- density per object, "must" be <= rho_min (if DENSITY_CONVERSION_TCP06)
			rho_min = 0.00,  -- if rho <= rho_min, f == f_topo
			rho_max = 2.00,  -- if rho >= rho_max, f == f_flow

			updateInt  = 5,  -- number of sim-frames between grid updates
			updateMode = 0,  -- UPDATE_MODE_ALLATONCE
		},

		["dummy"] = {
		},
	},
}



local function AddObjects(paramsTable, teamID, gpx, gpz, gdx, gdz, M, N, A, B)
	-- add one MxN-sized group of <AxB>-spaced objects
	local objectsTable = paramsTable["objects"]
	local objectIndex = #objectsTable + 1

	for i = 1, M do
		for j = 1, N do
			objectsTable[objectIndex] = {
				def = "core_goliath",
				pos = {gpx + i * A, 0.0, gpz + j * B},
				dir = {gdx,         0.0, gdz        },
				tid = teamID,
			}
			objectIndex = objectIndex + 1
		end
	end
end

AddObjects(params, 1,           256.0,                           256.0,                  1.0,  1.0,  5,  5, 128.0, 128.0)
AddObjects(params, 2, (4096.0 - 256.0) - (5 * 128.0),            256.0,                 -1.0,  1.0,  5,  5, 128.0, 128.0)
AddObjects(params, 3,           256.0,                 (4096.0 - 256.0) - (5 * 128.0),   1.0, -1.0,  5,  5, 128.0, 128.0)
AddObjects(params, 4, (4096.0 - 256.0) - (5 * 128.0),  (4096.0 - 256.0) - (5 * 128.0),  -1.0, -1.0,  5,  5, 128.0, 128.0)

-- AddObjects(params, 5,           256.0  + (4 * 128.0), 1536.0,   1.0, 0.0,  1, 15, 128.0, 128.0)
-- AddObjects(params, 6, (4096.0 - 256.0) - (5 * 128.0), 1536.0,  -1.0, 0.0,  1, 15, 128.0, 128.0)
