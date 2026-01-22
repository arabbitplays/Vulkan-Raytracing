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

namespace RtEngine {
    void Engine::run(CliArguments cli_args) {
        parseCliArguments(cli_args);

        init();
        mainLoop();
        cleanup();
    }

    void Engine::init() {
        scene_manager = std::make_shared<SceneManager>();
        createWindow();
        createRenderer();
        createGuiManager();
        createEngineContext();
        createRunner();
    }

    void Engine::createWindow() {
        window = std::make_shared<Window>(1920, 1040);
    }

    void Engine::createRenderer() {
        vulkan_renderer = std::make_shared<VulkanRenderer>();

        renderer_options = std::make_shared<BaseOptions>();
        renderer_options->resources_dir = options->resources_dir;
        renderer_options->config_file = options->config_file;
        vulkan_renderer->init(renderer_options, window);
    }

    void Engine::createGuiManager() {
        guiManager = std::make_shared<GuiManager>(vulkan_renderer->getVulkanContext());

        auto options_window = std::make_shared<OptionsWindow>(vulkan_renderer->getPropertiesManager());
        options_window->addCallback([this](uint32_t flags) {
            runner->setUpdateFlags(flags);
        });
        guiManager->addWindow(options_window);

        auto inspector_window = std::make_shared<InspectorWindow>(scene_manager);
        inspector_window->addCallback([this](uint32_t flags) {
            runner->setUpdateFlags(flags);
        });
        guiManager->addWindow(inspector_window);

        auto hierarchy_window = std::make_shared<HierarchyWindow>(inspector_window, scene_manager);
        hierarchy_window->addCallback([this](uint32_t flags) {
            runner->setUpdateFlags(flags);
        });
        guiManager->addWindow(hierarchy_window);
    }

    void Engine::createEngineContext() {
        engine_context = std::make_shared<EngineContext>();
        engine_context->window = window;
        engine_context->renderer = vulkan_renderer;
        engine_context->texture_repository = vulkan_renderer->getTextureRepository();
        engine_context->mesh_repository = vulkan_renderer->getMeshRepository();
        engine_context->scene_manager = scene_manager;
        engine_context->input_manager = std::make_shared<InputManager>(window);
        engine_context->swapchain_manager = std::make_shared<SwapchainManager>(vulkan_renderer->getSwapchain());
    }

    void Engine::createRunner() {
        if (options->runner_type == OFFLINE) {
            runner = std::make_shared<Runner>(engine_context, guiManager, scene_manager);
            SPDLOG_INFO("Offline runner created");
        } else if (options->runner_type == REALTIME) {
            //vulkan_renderer = std::make_shared<RealtimeRunner>();
        } else if (options->runner_type == REFERENCE) {
            runner = std::make_shared<ReferenceRunner>(engine_context, guiManager, scene_manager);
            SPDLOG_INFO("Reference runner created");
        } else if (options->runner_type == REFERENCE) {
            //vulkan_renderer = std::make_shared<BenchmarkRunner>();
        } else {
            SPDLOG_ERROR("No runner created");
            return;
        }


    }

    void Engine::mainLoop() {
        while (window->is_open() && runner->isRunning()) {
            window->pollEvents();

            if (PathUtil::getFile(runner->getScenePath()) != renderer_options->curr_scene_name) {
                std::string path = renderer_options->resources_dir + "/scenes/" +
                           renderer_options->curr_scene_name;
                runner->loadScene(path);
            }
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
        guiManager->destroy();
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
