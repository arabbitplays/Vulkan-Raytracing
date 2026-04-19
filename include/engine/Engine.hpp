#ifndef VULKAN_RAYTRACING_ENGINE_HPP
#define VULKAN_RAYTRACING_ENGINE_HPP
#include <memory>

#include "GuiManager.hpp"
#include "RenderingManager.hpp"
#include "Runner.hpp"
#include "RaytracingRenderer.hpp"

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

        void createWindow();
        void createRenderer();
        void createGuiManager();

        void createEngineContext();

        void createRunner();

        void setupGui() const;

        void mainLoop();

        void finishFrame();

        void cleanup();

    private:
        std::shared_ptr<EngineOptions> options;
        std::shared_ptr<IProperties> config_properties;

        std::shared_ptr<EngineContext> engine_context;

        std::shared_ptr<Window> window;
        std::shared_ptr<SceneManager> scene_manager;
        std::shared_ptr<GuiManager> gui_manager;
        std::shared_ptr<RenderingManager> rendering_manager;
        std::shared_ptr<Runner> runner;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_ENGINE_HPP