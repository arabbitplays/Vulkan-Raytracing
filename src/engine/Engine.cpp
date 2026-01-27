//
// Created by oschdi on 18.01.26.
//

#include "../../include/engine/Engine.hpp"

#include "BenchmarkRunner.hpp"
#include "CommandLineParser.hpp"
#include "HierarchyWindow.hpp"
#include "InspectorWindow.hpp"
#include "PathUtil.hpp"
#include "RealtimeRunner.hpp"
#include "ReferenceRunner.hpp"
#include "YamlLoadProperties.hpp"

namespace RtEngine {
    void Engine::run(CliArguments cli_args) {
        parseCliArguments(cli_args);

        init();
        mainLoop();
        cleanup();
    }

    void Engine::init() {
        config_properties = std::make_shared<YamlLoadProperties>(options->config_file);

        createWindow();
        createRenderer();
        createEngineContext();
        createGuiManager();
        createRunner();

        setupGui();
    }

    void Engine::createWindow() {
        window = std::make_shared<Window>(1920, 1040);
    }

    void Engine::createRenderer() {
        vulkan_renderer = std::make_shared<VulkanRenderer>(options->resources_dir);
        vulkan_renderer->init(window);

        auto update_flags = std::make_shared<UpdateFlags>();
        vulkan_renderer->initProperties(config_properties, update_flags);
    }

    void Engine::createGuiManager() {
        auto gui_renderer = std::make_shared<GuiRenderer>(vulkan_renderer->getVulkanContext());
        gui_manager = std::make_shared<GuiManager>(scene_manager, gui_renderer);
        gui_manager->addCallbackToAll([&] (const UpdateFlagsHandle& update_flags) {
            runner->setUpdateFlags(update_flags);
        });
    }

    void Engine::createEngineContext() {
        engine_context = std::make_shared<EngineContext>();
        engine_context->window = window;
        engine_context->renderer = vulkan_renderer;
        engine_context->texture_repository = vulkan_renderer->getTextureRepository();
        engine_context->mesh_repository = vulkan_renderer->getMeshRepository();
        scene_manager = std::make_shared<SceneManager>(options->resources_dir); // this is the non interfaced version
        engine_context->scene_manager = scene_manager; // this it the version for the components providing scene information
        engine_context->input_manager = std::make_shared<InputManager>(window);
        engine_context->swapchain_manager = std::make_shared<SwapchainManager>(vulkan_renderer->getSwapchain());
    }

    void Engine::createRunner() {
        std::shared_ptr<GuiRenderer> gui_renderer = gui_manager->getGuiRenderer();
        if (options->runner_type == OFFLINE) {
            runner = std::make_shared<Runner>(engine_context, gui_renderer, scene_manager);
            SPDLOG_INFO("Offline runner created");
        } else if (options->runner_type == REALTIME) {
            //vulkan_renderer = std::make_shared<RealtimeRunner>();
        } else if (options->runner_type == REFERENCE) {
            runner = std::make_shared<ReferenceRunner>(engine_context, gui_renderer, scene_manager);
            SPDLOG_INFO("Reference runner created");
        } else if (options->runner_type == BENCHMARK) {
            runner = std::make_shared<BenchmarkRunner>(engine_context, gui_renderer, scene_manager);
            SPDLOG_INFO("Benchmark runner created");
        } else {
            SPDLOG_ERROR("No runner created");
            return;
        }

        auto update_flags = std::make_shared<UpdateFlags>();
        runner->initProperties(config_properties, update_flags);
        runner->setUpdateFlags(update_flags);
    }

    void Engine::setupGui() const {
        gui_manager->options_window->addSerializable(runner);
        gui_manager->options_window->addSerializable(vulkan_renderer);
    }

    void Engine::mainLoop() {
        while (window->is_open() && runner->isRunning()) {
            window->pollEvents();

            runner->renderScene();
            finishFrame();
        }
    }

    void Engine::finishFrame() {
        engine_context->input_manager->reset();
    }

    void Engine::cleanup() {
        vulkan_renderer->waitForIdle();
        scene_manager->getCurrentScene()->destroy();
        gui_manager->destroy();
        vulkan_renderer->cleanup();
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
