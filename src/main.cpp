#include <exception>
#include <iostream>
#include "rendering/engine/VulkanEngine.hpp"
#include <CommandLineParser.hpp>

int main(int argc, char* argv[]) {
    CommandLineParser cli_parser = CommandLineParser();
    int32_t samples = 0;
    bool help = false, verbose = false;

    cli_parser.addFlag("--help", &help, "Show this message.");
    cli_parser.addInt("--samples", &samples, "Number of samples taken per pixel.");
    cli_parser.addFlag("-v", &verbose, "Debug messages should be displayed.");
    cli_parser.parse(argc, argv);

    if (help) {
        cli_parser.printHelp();
        return 0;
    }

    if (verbose) {
        spdlog::set_level(spdlog::level::debug);
    }

    VulkanEngine app;
    try {
        app.run();
    } catch (const std::exception& e) {
        spdlog::error(e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
