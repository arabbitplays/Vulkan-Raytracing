#include <exception>
#include <iostream>
#include "rendering/engine/VulkanEngine.hpp"
#include <CommandLineParser.hpp>

int main(int argc, char* argv[]) {
    RendererOptions options;

    CommandLineParser cli_parser = CommandLineParser();
    bool help = false, verbose = false;

    cli_parser.addFlag("--help", &help, "Show this message.");
    cli_parser.addInt("--samples", &options.sample_count, "Number of samples taken per pixel.");
    cli_parser.addString("--output", &options.output_path, "Generate one image and save it to the given file.");
    cli_parser.addFlag("-v", &verbose, "Display debug messages.");
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
        app.run(options);
    } catch (const std::exception& e) {
        spdlog::error(e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
