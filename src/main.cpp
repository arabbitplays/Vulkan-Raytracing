#include <BenchmarkRenderer.hpp>
#include <RealtimeRenderer.hpp>
#include <exception>
#include <iostream>
#include <VulkanEngine.hpp>
#include <CommandLineParser.hpp>
#include <filesystem>
#include <ReferenceRenderer.hpp>
using namespace RtEngine;

int main(int argc, char* argv[]) {
    std::string config_file, resources_dir;

    CommandLineParser cli_parser = CommandLineParser();
    bool help = false, verbose = false;
    bool reference = false, benchmark = false;

    cli_parser.addFlag("--help", &help, "Show this message.");
    cli_parser.addString("--resources", &resources_dir, "The path to the directory where all resource files can be found.");
    cli_parser.addString("--config", &config_file, "Path to the cofnig file.");
    cli_parser.addFlag("--ref", &reference, "Render a reference image.");
    cli_parser.addFlag("--benchmark", &benchmark, "Render an image and benchmark it against a reference.");
    cli_parser.addFlag("-v", &verbose, "Display debug messages.");
    cli_parser.parse(argc, argv);

    if (help) {
        cli_parser.printHelp();
        return 0;
    }

    if (benchmark && reference)
    {
        spdlog::error("Rendermode can not be reference AND benchmark!");
        return 1;
    }

    if (verbose) {
        spdlog::set_level(spdlog::level::debug);
    }

    std::shared_ptr<VulkanEngine> app;
    if (benchmark)
    {
        app = std::make_shared<BenchmarkRenderer>();
    } else if (reference)
    {
        app = std::make_shared<ReferenceRenderer>();
    } else
    {
        app = std::make_shared<RealtimeRenderer>();
    }

    try {
        app->run(config_file, resources_dir);
    } catch (const std::exception& e) {
        spdlog::error(e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
