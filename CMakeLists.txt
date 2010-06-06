PROJECT(CORPSE)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

#-------------------------------------------------------------------------------
# General Settings
#-------------------------------------------------------------------------------
SET(CMAKE_COLOR_MAKEFILE ON)
SET(CMAKE_MODULE_PATH "${CORPSE_SOURCE_DIR}/cmake/packages")


#-------------------------------------------------------------------------------
# Extract and set the version
#-------------------------------------------------------------------------------
EXECUTE_PROCESS(
	COMMAND "git describe --tags"
	OUTPUT_VARIABLE GIT_VERSION
	ERROR_VARIABLE GIT_ERROR
	OUTPUT_STRIP_TRAILING_WHITESPACE
	)

IF(GIT_VERSION)
	SET(VERSION ${GIT_VERSION})
ELSE(NOT GIT_VERSION)
	SET(VERSION "1.0.0")
ENDIF(GIT_VERSION)
MESSAGE("-- Corpse version set to ${VERSION}")

CONFIGURE_FILE(
	"${CORPSE_SOURCE_DIR}/cmake/Corpse.hpp.in"
	"${CORPSE_BINARY_DIR}/Corpse.hpp"
	)


#-------------------------------------------------------------------------------
# Include sources
#-------------------------------------------------------------------------------
INCLUDE(cmake/sources.list)
INCLUDE_DIRECTORIES(${CORPSE_BINARY_DIR})


#-------------------------------------------------------------------------------
# Add our own libraries
#-------------------------------------------------------------------------------
ADD_LIBRARY(Pathmodule ${PATHMODULE_SOURCE})


#-------------------------------------------------------------------------------
# Find 3rd party libraries and include their headers
#-------------------------------------------------------------------------------
FIND_PACKAGE(GLUT REQUIRED)
FIND_PACKAGE(SDL REQUIRED)
FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(GLEW REQUIRED)
FIND_PACKAGE(Lua51 REQUIRED)
FIND_PACKAGE(DevIL REQUIRED)

INCLUDE_DIRECTORIES(
	${GLUT_INCLUDE_DIR}
	${SDL_INCLUDE_DIR}
	${OPENGL_INCLUDE_DIR}
	${GLEW_INCLUDE_DIR}
	${LUA51_INCLUDE_DIR}
	${IL_INCLUDE_DIR}
	)


#-------------------------------------------------------------------------------
# Define executable and link libraries
#-------------------------------------------------------------------------------
ADD_EXECUTABLE(CORPSE ${CORPSE_SOURCE})

TARGET_LINK_LIBRARIES(CORPSE
	${SDL_LIBRARIES}
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES}
	${GLUT_LIBRARIES}
	${IL_LIBRARIES}
	${ILU_LIBRARIES}
	${LUA51_LIBRARIES}
	Pathmodule
	)
