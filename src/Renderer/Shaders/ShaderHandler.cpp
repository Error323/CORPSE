#include <GL/glew.h>
#include <cassert>

#include "./ShaderHandler.hpp"
#include "./Shader.hpp"
#include "../../System/EngineAux.hpp"
#include "../../System/FileHandler.hpp"
#include "../../System/Logger.hpp"

CShaderHandler* CShaderHandler::GetInstance() {
	static CShaderHandler shaHandler;
	return &shaHandler;
}



void CShaderHandler::ReleaseProgramObjects(const std::string& poClass) {
	if (programObjects.find(poClass) == programObjects.end()) {
		return;
	}

	for (ProgramObjMapIt it = programObjects[poClass].begin(); it != programObjects[poClass].end(); it++) {
		// free the program object and its attachments
		(it->second)->Release(); delete (it->second);
	}

	programObjects[poClass].clear();
	programObjects.erase(poClass);
}

Shader::IProgramObject* CShaderHandler::CreateProgramObject(const std::string& poClass, const std::string& poName, bool arbProgram) {
	Shader::IProgramObject* po = NULL;

	if (programObjects.find(poClass) != programObjects.end()) {
		if (programObjects[poClass].find(poName) != programObjects[poClass].end()) {
			return (programObjects[poClass][poName]);
		}
	} else {
		programObjects[poClass] = ProgramObjMap();
	}

	if (arbProgram) {
		po = new Shader::ARBProgramObject();
	} else {
		po = new Shader::GLSLProgramObject();
	}

	programObjects[poClass][poName] = po;
	return po;
}

Shader::IProgramObject* CShaderHandler::CreateProgramObject(
	const std::string& poClass,
	const std::string& poName,
	const std::string& vsStr,
	const std::string& vsDefs,
	const std::string& fsStr,
	const std::string& fsDefs,
	bool arbProgram
) {
	Shader::IProgramObject* po = CreateProgramObject(poClass, poName, arbProgram);

	if (po->IsValid()) {
		return po;
	}

	Shader::IShaderObject* vso = CreateShaderObject(vsStr, vsDefs, (arbProgram? GL_VERTEX_PROGRAM_ARB: GL_VERTEX_SHADER));
	Shader::IShaderObject* fso = CreateShaderObject(fsStr, fsDefs, (arbProgram? GL_FRAGMENT_PROGRAM_ARB: GL_FRAGMENT_SHADER));

	po->AttachShaderObject(vso);
	po->AttachShaderObject(fso);

	po->Link();

	if (!po->IsValid()) {
		LOG << "[CShaderHandler::CreateProgramObject]\n";
		LOG << "\tprogram-object name: " << poName << "\n";
		LOG << "\tlink-log:\n%s\n" << po->GetLog() << "\n";
	}
	return po;
}



Shader::IShaderObject* CShaderHandler::CreateShaderObject(const std::string& soName, const std::string& soDefs, int soType) {
	assert(!soName.empty());

	bool arbShader = false;

	std::string soSource("");
	CFileHandler soFile(soName);

	if (soFile.FileExists()) {
		arbShader =
			soName.find(".glsl") == std::string::npos &&
			soName.find(".vert") == std::string::npos &&
			soName.find(".frag") == std::string::npos;

		std::vector<char> soFileBuffer(soFile.FileSize() + 1, 0);
		soFile.Read(&soFileBuffer[0], soFile.FileSize());

		soSource = std::string(&soFileBuffer[0]);
	} else {
		arbShader =
			(soName.find("!!ARBvp") != std::string::npos) ||
			(soName.find("!!ARBfp") != std::string::npos);
		soSource = soName;
	}

	if (!arbShader) {
		soSource = soDefs + soSource;
	}

	Shader::IShaderObject* so = NULL;

	switch (soType) {
		case GL_VERTEX_PROGRAM_ARB:
		case GL_FRAGMENT_PROGRAM_ARB: {
			assert(arbShader);
			so = new Shader::ARBShaderObject(soType, soSource);
		} break;

		case GL_VERTEX_SHADER:
		case GL_FRAGMENT_SHADER: {
			assert(!arbShader);
			so = new Shader::GLSLShaderObject(soType, soSource);
		} break;
	}

	assert(so != NULL);
	so->Compile();

	if (!so->IsValid()) {
		LOG << "[CShaderHandler::CreateShaderObject]\n";
		LOG << "\tshader-object name: " << soName << "\n";
		LOG << "\tlink-log:\n%s\n" << so->GetLog() << "\n";
	}
	return so;
}
