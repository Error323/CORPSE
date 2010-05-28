// g++ -Wall -Wextra -g -o tst  TestModelLoader.cpp  ModelReaderS3O.cpp ../../System/FileHandler.cpp

#include "./Format3DO.hpp"
#include "./FormatS3O.hpp"
#include "./ModelReader3DO.hpp"
#include "./ModelReaderS3O.hpp"

int main(int argc, char** argv) {
	if (argc == 3) {
		// ex. "Commander.s3o"
		std::string modelName(argv[1]);
		std::string modelType(argv[2]);

		if (modelType == "--3do") {
			// CModelReader3DO m;
			// m.Load(modelName);
		}

		if (modelType == "--s3o") {
			CModelReaderS3O m;
			m.Load(modelName);
		}
	}

	return 0;
}
