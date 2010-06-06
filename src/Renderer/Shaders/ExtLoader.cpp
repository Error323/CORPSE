#include <iostream>

#include <GL/glew.h>
#include <GL/glx.h>
#include <GL/glxext.h>

#include "./ExtLoader.hpp"



// OpenGL 2.0 functions
PFNGLCREATEPROGRAMPROC				glCreateProgram = 0x0;
PFNGLCREATESHADERPROC				glCreateShader = 0x0;
PFNGLSHADERSOURCEPROC				glShaderSource = 0x0;
PFNGLCOMPILESHADERPROC				glCompileShader = 0x0;
// PFNGLGETOBJECTPARAMETERIVPROC	glGetObjectParameteriv = 0x0;
PFNGLGETSHADERIVPROC				glGetShaderiv = 0x0;
PFNGLGETPROGRAMIVPROC				glGetProgramiv = 0x0;
PFNGLATTACHSHADERPROC				glAttachShader = 0x0;
PFNGLDETACHSHADERPROC				glDetachShader = 0x0;
PFNGLGETSHADERINFOLOGPROC			glGetShaderInfoLog = 0x0;
PFNGLGETPROGRAMINFOLOGPROC			glGetProgramInfoLog = 0x0;
PFNGLLINKPROGRAMPROC				glLinkProgram = 0x0;
PFNGLUSEPROGRAMPROC					glUseProgram = 0x0;
PFNGLVALIDATEPROGRAMPROC			glValidateProgram = 0x0;
PFNGLGETUNIFORMLOCATIONPROC			glGetUniformLocation = 0x0;
PFNGLGETATTRIBLOCATIONPROC			glGetAttribLocation = 0x0;
PFNGLUNIFORM1FPROC					glUniform1f = 0x0;
PFNGLUNIFORM1IPROC					glUniform1i = 0x0;
PFNGLVERTEXATTRIB1FPROC				glVertexAttrib1f = 0x0;
PFNGLDELETESHADERPROC				glDeleteShader = 0x0;
PFNGLDELETEPROGRAMPROC				glDeleteProgram = 0x0;


CExtLoader::CExtLoader() {
	std::string glVersion((const char*) glGetString(GL_VERSION));
	std::string glVendor((const char*) glGetString(GL_VENDOR));
	std::string glRenderer((const char*) glGetString(GL_RENDERER));

	std::cout << std::endl;
	std::cout << "GL version:  " << glVersion << std::endl;
	std::cout << "GL vendor:   " << glVendor << std::endl;
	std::cout << "GL renderer: " << glRenderer << std::endl;
	std::cout << std::endl;

	haveOGL2 = (glVersion[0] == '2');
}

bool CExtLoader::LoadExtensions() {
	// "glXGetProcAddressARB’ was not declared in this scope"
	#define glXGetProcAddr(x) (*glXGetProcAddress) ((const GLubyte*) x)

	if (haveOGL2) {
		glCreateProgram = (PFNGLCREATEPROGRAMPROC) glXGetProcAddr("glCreateProgram");
		glCreateShader = (PFNGLCREATESHADERPROC) glXGetProcAddr("glCreateShader");
		glShaderSource = (PFNGLSHADERSOURCEPROC) glXGetProcAddr("glShaderSource");
		glCompileShader = (PFNGLCOMPILESHADERPROC) glXGetProcAddr("glCompileShader");
		glGetShaderiv = (PFNGLGETSHADERIVPROC) glXGetProcAddr("glGetShaderiv");
		glGetProgramiv = (PFNGLGETPROGRAMIVPROC) glXGetProcAddr("glGetProgramiv");
		glAttachShader = (PFNGLATTACHSHADERPROC) glXGetProcAddr("glAttachShader");
		glDetachShader = (PFNGLDETACHSHADERPROC) glXGetProcAddr("glDetachShader");
		glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC) glXGetProcAddr("glGetShaderInfoLog");
		glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC) glXGetProcAddr("glGetProgramInfoLog");
		glLinkProgram = (PFNGLLINKPROGRAMPROC) glXGetProcAddr("glLinkProgram");
		glUseProgram = (PFNGLUSEPROGRAMPROC) glXGetProcAddr("glUseProgram");
		glValidateProgram = (PFNGLVALIDATEPROGRAMPROC) glXGetProcAddr("glValidateProgram");
		glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC) glXGetProcAddr("glGetUniformLocation");
		glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC) glXGetProcAddr("glGetAttribLocation");
		glUniform1f = (PFNGLUNIFORM1FPROC) glXGetProcAddr("glUniform1f");
		glUniform1i = (PFNGLUNIFORM1IPROC) glXGetProcAddr("glUniform1i");
		glVertexAttrib1f = (PFNGLVERTEXATTRIB1FPROC) glXGetProcAddr("glVertexAttrib1f");
		glDeleteShader = (PFNGLDELETESHADERPROC) glXGetProcAddr("glDeleteShader");
		glDeleteProgram = (PFNGLDELETEPROGRAMPROC) glXGetProcAddr("glDeleteProgram");

		std::cout << "glCreateProgram:      " << glCreateProgram << std::endl;
		std::cout << "glCreateShader:       " << glCreateShader << std::endl;
		std::cout << "glShaderSource:       " << glShaderSource << std::endl;
		std::cout << "glCompileShader:      " << glCompileShader << std::endl;
		std::cout << "glGetShaderiv:        " << glGetShaderiv << std::endl;
		std::cout << "glGetProgramiv:       " << glGetProgramiv << std::endl;
		std::cout << "glAttachShader:       " << glAttachShader << std::endl;
		std::cout << "glDetachShader:       " << glDetachShader << std::endl;
		std::cout << "glGetShaderInfoLog:   " << glGetShaderInfoLog << std::endl;
		std::cout << "glGetProgramInfoLog:  " << glGetProgramInfoLog << std::endl;
		std::cout << "glLinkProgram:        " << glLinkProgram << std::endl;
		std::cout << "glUseProgram:         " << glUseProgram << std::endl;
		std::cout << "glValidateProgram:    " << glValidateProgram << std::endl;
		std::cout << "glGetUniformLocation: " << glGetUniformLocation << std::endl;
		std::cout << "glGetAttribLocation:  " << glGetUniformLocation << std::endl;
		std::cout << "glUniform1f:          " << glUniform1f << std::endl;
		std::cout << "glVertexAttrib1f:     " << glVertexAttrib1f << std::endl;
		std::cout << "glDeleteShader:       " << glDeleteShader << std::endl;
		std::cout << "glDeleteProgram:      " << glDeleteProgram << std::endl;

		return true;
	} else {
		std::cout << "driver does not support OpenGL 2.0 or greater, cannot proceed" << std::endl;

		/*
		glCreateProgramObjectARB = (PFNGLCREATEPROGRAMOBJECTARBPROC) glXGetProcAddr("glCreateProgramObjectARB");
		glCreateShaderObjectARB = (PFNGLCREATESHADEROBJECTARBPROC) glXGetProcAddr("glCreateShaderObjectARB");
		glShaderSourceARB = (PFNGLSHADERSOURCEARBPROC) glXGetProcAddr("glShaderSourceARB");
		glCompileShaderARB = (PFNGLCOMPILESHADERARBPROC) glXGetProcAddr("glCompileShaderARB");
		glGetObjectParameterivARB = (PFNGLGETOBJECTPARAMETERIVARBPROC) glXGetProcAddr("glGetObjectParameterivARB");
		glAttachObjectARB = (PFNGLATTACHOBJECTARBPROC) glXGetProcAddr("glAttachObjectARB");
		glDetachObjectARB = (PFNGLDETACHOBJECTARBPROC) glXGetProcAddr("glDetachObjectARB");
		glDeleteObjectARB = (PFNGLDELETEOBJECTARBPROC) glXGetProcAddr("glDeleteObjectARB");
		glGetInfoLogARB = (PFNGLGETINFOLOGARBPROC) glXGetProcAddr("glGetInfoLogARB");
		glLinkProgramARB = (PFNGLLINKPROGRAMARBPROC) glXGetProcAddr("glLinkProgramARB");
		glUseProgramObjectARB = (PFNGLUSEPROGRAMOBJECTARBPROC) glXGetProcAddr("glUseProgramObjectARB");
		glGetUniformLocationARB = (PFNGLGETUNIFORMLOCATIONARBPROC) glXGetProcAddr("glGetUniformLocationARB");
		glUniform1fARB = (PFNGLUNIFORM1FARBPROC) glXGetProcAddr("glUniform1fARB");
		glUniform1iARB = (PFNGLUNIFORM1IARBPROC) glXGetProcAddr("glUniform1iARB");
		*/

		return false;
	}
}
