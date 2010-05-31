SRC_DIR = ./src
OBJ_DIR = ./obj
BIN_DIR = ./bin

ENGINE_TARGET = PathEngine
MODULE_TARGET = PathModule

MKDIR = mkdir

CC = g++
CFLAGS = -Wall -Wextra -g -O2 -fno-strict-aliasing
LFLAGS_ENGINE = -lSDL -lGL -lGLEW -lGLU -lglut -lIL -lILU -llua5.1   -L$(BIN_DIR) -l$(MODULE_TARGET)
LFLAGS_MODULE = -shared

BASE_SRC_DIR     = $(SRC_DIR)
BASE_OBJ_DIR     = $(OBJ_DIR)
EXT_SRC_DIR      = $(BASE_SRC_DIR)/Ext
EXT_OBJ_DIR      = $(BASE_OBJ_DIR)/Ext
INPUT_SRC_DIR    = $(BASE_SRC_DIR)/Input
INPUT_OBJ_DIR    = $(BASE_OBJ_DIR)/Input
MAP_SRC_DIR      = $(BASE_SRC_DIR)/Map
MAP_OBJ_DIR      = $(BASE_OBJ_DIR)/Map
MAP_SMF_SRC_DIR  = $(MAP_SRC_DIR)/SMF
MAP_SMF_OBJ_DIR  = $(MAP_OBJ_DIR)/SMF
MATH_SRC_DIR     = $(BASE_SRC_DIR)/Math
MATH_OBJ_DIR     = $(BASE_OBJ_DIR)/Math
PATH_SRC_DIR     = $(BASE_SRC_DIR)/Path
PATH_OBJ_DIR     = $(BASE_OBJ_DIR)/Path
RENDERER_SRC_DIR = $(BASE_SRC_DIR)/Renderer
RENDERER_OBJ_DIR = $(BASE_OBJ_DIR)/Renderer
MODELS_SRC_DIR   = $(RENDERER_SRC_DIR)/Models
MODELS_OBJ_DIR   = $(RENDERER_OBJ_DIR)/Models
SHADERS_SRC_DIR  = $(RENDERER_SRC_DIR)/Shaders
SHADERS_OBJ_DIR  = $(RENDERER_OBJ_DIR)/Shaders
TEXTURES_SRC_DIR = $(RENDERER_SRC_DIR)/Textures
TEXTURES_OBJ_DIR = $(RENDERER_OBJ_DIR)/Textures
ENV_SRC_DIR      = $(RENDERER_SRC_DIR)/Env
ENV_OBJ_DIR      = $(RENDERER_OBJ_DIR)/Env
SIM_SRC_DIR      = $(BASE_SRC_DIR)/Sim
SIM_OBJ_DIR      = $(BASE_OBJ_DIR)/Sim
SYSTEM_SRC_DIR   = $(BASE_SRC_DIR)/System
SYSTEM_OBJ_DIR   = $(BASE_OBJ_DIR)/System

EXT_OBS = $(EXT_OBJ_DIR)/CallOutHandler.o
INPUT_OBS = $(INPUT_OBJ_DIR)/InputHandler.o
MAP_OBS = \
	$(MAP_OBJ_DIR)/BaseGroundDrawer.o \
	$(MAP_OBJ_DIR)/Ground.o \
	$(MAP_OBJ_DIR)/MapInfo.o \
	$(MAP_OBJ_DIR)/ReadMap.o \
	$(MAP_OBJ_DIR)/MetalMap.o
MAP_SMF_OBS = \
	$(MAP_SMF_OBJ_DIR)/SMFReadMap.o \
	$(MAP_SMF_OBJ_DIR)/SMFMapFile.o \
	$(MAP_SMF_OBJ_DIR)/SMFGroundDrawer.o \
	$(MAP_SMF_OBJ_DIR)/SMFGroundTextures.o
MATH_OBS = \
	$(MATH_OBJ_DIR)/vec3.o \
	$(MATH_OBJ_DIR)/mat33.o \
	$(MATH_OBJ_DIR)/mat44.o
PATH_OBS = \
	$(PATH_OBJ_DIR)/PathModule.o
RENDERER_OBS = \
	$(RENDERER_OBJ_DIR)/RenderThread.o \
	$(RENDERER_OBJ_DIR)/SMFRenderer.o \
	$(RENDERER_OBJ_DIR)/SprVertexArray.o \
	$(RENDERER_OBJ_DIR)/Scene.o \
	$(RENDERER_OBJ_DIR)/GL.o \
	$(RENDERER_OBJ_DIR)/GLObjects.o \
	$(RENDERER_OBJ_DIR)/Camera.o \
	$(RENDERER_OBJ_DIR)/CameraController.o
MODELS_OBS = \
	$(MODELS_OBJ_DIR)/ModelDrawerS3O.o \
	$(MODELS_OBJ_DIR)/ModelReaderS3O.o \
	$(MODELS_OBJ_DIR)/ModelReader3DO.o
SHADERS_OBS = \
	$(SHADERS_OBJ_DIR)/ShaderHandler.o \
	$(SHADERS_OBJ_DIR)/Shader.o
TEXTURES_OBS = \
	$(TEXTURES_OBJ_DIR)/BitMap.o \
	$(TEXTURES_OBJ_DIR)/NVDDS.o \
	$(TEXTURES_OBJ_DIR)/TextureHandlerS3O.o
ENV_OBS = \
	$(ENV_OBJ_DIR)/SkyBox.o \
	$(ENV_OBJ_DIR)/ShadowHandler.o
SIM_OBS = \
	$(SIM_OBJ_DIR)/SimThread.o \
	$(SIM_OBJ_DIR)/SimObject.o \
	$(SIM_OBJ_DIR)/SimObjectDefLoader.o \
	$(SIM_OBJ_DIR)/SimObjectHandler.o
SYSTEM_OBS = \
	$(SYSTEM_OBJ_DIR)/Client.o \
	$(SYSTEM_OBJ_DIR)/Server.o \
	$(SYSTEM_OBJ_DIR)/NetMessageBuffer.o \
	$(SYSTEM_OBJ_DIR)/EventHandler.o \
	$(SYSTEM_OBJ_DIR)/IEvent.o \
	$(SYSTEM_OBJ_DIR)/Engine.o \
	$(SYSTEM_OBJ_DIR)/EngineAux.o \
	$(SYSTEM_OBJ_DIR)/FileHandler.o \
	$(SYSTEM_OBJ_DIR)/ScopedTimer.o \
	$(SYSTEM_OBJ_DIR)/LuaParser.o \
	$(SYSTEM_OBJ_DIR)/Logger.o \
	$(SYSTEM_OBJ_DIR)/Main.o

MODULE_OBJECTS = \
	$(PATH_OBS)

ENGINE_OBJECTS = \
	$(EXT_OBS) \
	$(INPUT_OBS) \
	$(MAP_OBS) \
	$(MAP_SMF_OBS) \
	$(MATH_OBS) \
	$(RENDERER_OBS) \
	$(MODELS_OBS) \
	$(SHADERS_OBS) \
	$(TEXTURES_OBS) \
	$(ENV_OBS) \
	$(SIM_OBS) \
	$(SYSTEM_OBS)



$(EXT_OBJ_DIR)/%.o: $(EXT_SRC_DIR)/%.cpp
	$(CC) $(CFLAGS) -c   -o $@    $<

$(INPUT_OBJ_DIR)/%.o: $(INPUT_SRC_DIR)/%.cpp
	$(CC) $(CFLAGS) -c   -o $@    $<

$(MAP_OBJ_DIR)/%.o: $(MAP_SRC_DIR)/%.cpp
	$(CC) $(CFLAGS) -c   -o $@    $<

$(MAP_SMF_OBJ_DIR)/%.o: $(MAP_SMF_SRC_DIR)/%.cpp
	$(CC) $(CFLAGS) -c   -o $@    $<

$(MATH_OBJ_DIR)/%.o: $(MATH_SRC_DIR)/%.cpp
	$(CC) $(CFLAGS) -c   -o $@    $<

$(PATH_OBJ_DIR)/%.o: $(PATH_SRC_DIR)/%.cpp
	$(CC) $(CFLAGS) -c   -o $@    $<

$(RENDERER_OBJ_DIR)/%.o: $(RENDERER_SRC_DIR)/%.cpp
	$(CC) $(CFLAGS) -c   -o $@    $<

$(MODELS_OBJ_DIR)/%.o: $(MODELS_SRC_DIR)/%.cpp
	$(CC) $(CFLAGS) -c   -o $@    $<

$(SHADERS_OBJ_DIR)/%.o: $(SHADERS_SRC_DIR)/%.cpp
	$(CC) $(CFLAGS) -c   -o $@    $<

$(TEXTURES_OBJ_DIR)/%.o: $(TEXTURES_SRC_DIR)/%.cpp
	$(CC) $(CFLAGS) -c   -o $@    $<

$(ENV_OBJ_DIR)/%.o: $(ENV_SRC_DIR)/%.cpp
	$(CC) $(CFLAGS) -c   -o $@    $<

$(SIM_OBJ_DIR)/%.o: $(SIM_SRC_DIR)/%.cpp
	$(CC) $(CFLAGS) -c   -o $@    $<

$(SYSTEM_OBJ_DIR)/%.o: $(SYSTEM_SRC_DIR)/%.cpp
	$(CC) $(CFLAGS) -c   -o $@    $<


default:
	make dirs
	make moduleObjects
	make moduleTarget
	make engineObjects
	make engineTarget

dirs:
	if [ ! -d $(BIN_DIR) ]; then $(MKDIR) $(BIN_DIR); fi
	if [ ! -d $(OBJ_DIR) ]; then $(MKDIR) $(OBJ_DIR); fi

	if [ ! -d $(EXT_OBJ_DIR) ];      then $(MKDIR) $(EXT_OBJ_DIR); fi
	if [ ! -d $(INPUT_OBJ_DIR) ];    then $(MKDIR) $(INPUT_OBJ_DIR); fi
	if [ ! -d $(MAP_OBJ_DIR) ];      then $(MKDIR) $(MAP_OBJ_DIR); fi
	if [ ! -d $(MAP_SMF_OBJ_DIR) ];  then $(MKDIR) $(MAP_SMF_OBJ_DIR); fi
	if [ ! -d $(MATH_OBJ_DIR) ];     then $(MKDIR) $(MATH_OBJ_DIR); fi
	if [ ! -d $(PATH_OBJ_DIR) ];     then $(MKDIR) $(PATH_OBJ_DIR); fi
	if [ ! -d $(RENDERER_OBJ_DIR) ]; then $(MKDIR) $(RENDERER_OBJ_DIR); fi
	if [ ! -d $(MODELS_OBJ_DIR) ];   then $(MKDIR) $(MODELS_OBJ_DIR); fi
	if [ ! -d $(SHADERS_OBJ_DIR) ];  then $(MKDIR) $(SHADERS_OBJ_DIR); fi
	if [ ! -d $(TEXTURES_OBJ_DIR) ]; then $(MKDIR) $(TEXTURES_OBJ_DIR); fi
	if [ ! -d $(ENV_OBJ_DIR) ];      then $(MKDIR) $(ENV_OBJ_DIR); fi
	if [ ! -d $(SIM_OBJ_DIR) ];      then $(MKDIR) $(SIM_OBJ_DIR); fi
	if [ ! -d $(SYSTEM_OBJ_DIR) ];   then $(MKDIR) $(SYSTEM_OBJ_DIR); fi

moduleObjects: $(MODULE_OBJECTS)
moduleTarget:
	$(CC) $(LFLAGS_MODULE) -o $(BIN_DIR)/lib$(MODULE_TARGET).so    $(MODULE_OBJECTS)

engineObjects: $(ENGINE_OBJECTS)
engineTarget:
	$(CC) $(LFLAGS_ENGINE) -o $(BIN_DIR)/$(ENGINE_TARGET).bin    $(ENGINE_OBJECTS)

clean:
	rm -rf $(BIN_DIR)
	rm -rf $(OBJ_DIR)
