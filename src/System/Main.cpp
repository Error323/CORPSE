#include <cstdio>

#include "./Engine.hpp"
#include "./CORPSE.hpp"

int main(int argc, char** argv) {
	printf("\n[%s] %s\n\n", __FUNCTION__, HUMAN_NAME);

	CEngine* engine;

	engine = CEngine::GetInstance(argc, argv);
	engine->Run();

	CEngine::FreeInstance(engine);
	return 0;
}
