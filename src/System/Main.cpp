#include "./Engine.hpp"
#include "./CORPSE.hpp"

#include <stdio.h>

int main(int argc, char** argv) {
	printf("\n%s\n\n",HUMAN_NAME);
	CEngine* engine;

	engine = CEngine::GetInstance(argc, argv);
	engine->Run();

	CEngine::FreeInstance(engine);
	return 0;
}
