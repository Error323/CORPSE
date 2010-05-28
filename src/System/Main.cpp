#include "./Engine.hpp"

int main(int argc, char** argv) {
	CEngine* engine;

	engine = CEngine::GetInstance(argc, argv);
	engine->Run();

	CEngine::FreeInstance(engine);
	return 0;
}
