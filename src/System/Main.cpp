#include <iostream>

#include "./Engine.hpp"
#include "Corpse.hpp"

int main(int argc, char** argv) {
	std::cout << HUMAN_NAME << std::endl;
	CEngine* engine;

	engine = CEngine::GetInstance(argc, argv);
	engine->Run();

	CEngine::FreeInstance(engine);
	return 0;
}
