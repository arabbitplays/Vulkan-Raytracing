#include <exception>
#include <iostream>
#include "rendering/engine/VulkanEngine.hpp"
#include <CommandLineParser.hpp>
#include <filesystem>

int main(int argc, char* argv[]) {
    RendererOptions options;

    CommandLineParser cli_parser = CommandLineParser();
    bool help = false, verbose = false;

    cli_parser.addFlag("--help", &help, "Show this message.");
    cli_parser.addInt("--samples", &options.sample_count, "Number of samples taken per pixel.");
    cli_parser.addString("--output", &options.output_path, "Generate one image and save it to the given file.");
    cli_parser.addString("--scenes", &options.scene_dir_path, "The path to the directory where the scene files can be found.");
    cli_parser.addFlag("-v", &verbose, "Display debug messages.");
    cli_parser.parse(argc, argv);

    if (help) {
        cli_parser.printHelp();
        return 0;
    }

    if (verbose) {
        spdlog::set_level(spdlog::level::debug);
    }

    try {
        for (const auto& entry : std::filesystem::directory_iterator(options.scene_dir_path)) {
            options.scene_paths.push_back(entry.path().filename());
        }
    } catch (const std::exception& e) {
        spdlog::error(e.what());
        return 1;
    }

    if (options.scene_paths.empty())
    {
        spdlog::error("No scenes found in scene directory " + options.scene_dir_path + ".");
        return 1;
    }
    options.curr_scene_path = options.scene_paths[0];

    VulkanEngine app;
    try {
        app.run(options);
    } catch (const std::exception& e) {
        spdlog::error(e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
