//
// Created by oschdi on 18.01.26.
//

#ifndef VULKAN_RAYTRACING_ENGINE_HPP
#define VULKAN_RAYTRACING_ENGINE_HPP
#include <memory>

#include "Runner.hpp"
#include "VulkanRenderer.hpp"

namespace RtEngine {
    enum RunnerType {
        NONE,
        REALTIME,
        OFFLINE,
        REFERENCE,
        BENCHMARK,
    };

    struct EngineOptions {
        std::string config_file, resources_dir;
        bool verbose = false;
        RunnerType runner_type = NONE;
    };

    struct CliArguments {
        int argc;
        char **argv;

        CliArguments() = default;

        CliArguments(int argc, char ** argv) : argc(argc), argv(argv) {}
    };

    class Engine {
    public:
        Engine() = default;

        void run(CliArguments cli_args);

        void init();

        void parseCliArguments(CliArguments cli_args);

        void mainLoop();

        void createRenderer();

        void createRunner();

        void cleanup();

    private:
        std::shared_ptr<EngineOptions> options;
        std::shared_ptr<BaseOptions> renderer_options;

        std::shared_ptr<Window> window;
        std::shared_ptr<VulkanRenderer> vulkan_renderer;
        std::shared_ptr<Runner> runner;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_ENGINE_HPP