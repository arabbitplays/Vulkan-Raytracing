#include <exception>
#include <filesystem>
#include <iostream>

#include "Engine.hpp"

namespace RtEngine {
	class Engine;
}

using namespace RtEngine;

int main(int argc, char *argv[]) {
	std::shared_ptr<Engine> engine = std::make_shared<Engine>();
	CliArguments args{argc, argv};
	try {
		engine->run(args);
	} catch (const std::exception &e) {
		spdlog::error(e.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
