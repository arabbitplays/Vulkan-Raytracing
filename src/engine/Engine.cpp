//
// Created by oschdi on 18.01.26.
//

#include "../../include/engine/Engine.hpp"

#include "BenchmarkRunner.hpp"
#include "CommandLineParser.hpp"
#include "RealtimeRunner.hpp"
#include "ReferenceRunner.hpp"

namespace RtEngine {
    void Engine::run(CliArguments cli_args) {
        parseCliArguments(cli_args);

        if (options->runner_type == OFFLINE) {
            vulkan_renderer = std::make_shared<VulkanRenderer>();
        } else if (options->runner_type == REALTIME) {
            vulkan_renderer = std::make_shared<RealtimeRunner>();
        } else if (options->runner_type == REFERENCE) {
            vulkan_renderer = std::make_shared<ReferenceRunner>();
        } else if (options->runner_type == REFERENCE) {
            vulkan_renderer = std::make_shared<BenchmarkRunner>();
        } else {
            return;
        }

        init();
    }

    void Engine::init() {
        window = std::make_shared<Window>(1920, 1040);

        auto renderer_options = std::make_shared<BaseOptions>();
        renderer_options->resources_dir = options->resources_dir;
        renderer_options->config_file = options->config_file;
        vulkan_renderer->init(renderer_options, window);
    }

    void Engine::mainLoop() {

    }


    void Engine::parseCliArguments(CliArguments cli_args) {
        options = std::make_shared<EngineOptions>();

        CommandLineParser cli_parser = CommandLineParser();

        bool help = false;
        bool reference = false, benchmark = false, realtime = false;

        cli_parser.addFlag("--help", &help, "Show this message.");
        cli_parser.addString("--resources", &options->resources_dir,
                             "The path to the directory where all resource files can be found.");
        cli_parser.addString("--config", &options->config_file, "Path to the cofnig file.");
        cli_parser.addFlag("--ref", &reference, "Render a reference image.");
        cli_parser.addFlag("--benchmark", &benchmark, "Render an image and benchmark it against a reference.");
        cli_parser.addFlag("--realtime", &realtime, "Render an image in realtime.");
        cli_parser.addFlag("-v", &options->verbose, "Display debug messages.");
        cli_parser.parse(cli_args.argc, cli_args.argv);

        if (help) {
            cli_parser.printHelp();
            return;
        }

        if (options->verbose) {
            spdlog::set_level(spdlog::level::debug);
        }

        if (benchmark) {
            options->runner_type = BENCHMARK;
        } else if (reference) {
            options->runner_type = REFERENCE;
        } else if (realtime) {
            options->runner_type = REALTIME;
        } else {
            options->runner_type = OFFLINE;
        }
    }
} // RtEngine
