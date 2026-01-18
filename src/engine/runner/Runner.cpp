//
// Created by oschdi on 18.01.26.
//

#include "../../../include/engine/runner/Runner.hpp"

namespace RtEngine {
    Runner::Runner(std::shared_ptr<EngineContext> engine_context, std::shared_ptr<GuiManager> gui_manager, std::shared_ptr<SceneManager> scene_manager) : renderer(engine_context->renderer), gui_manager(gui_manager), scene_manager(scene_manager) {
        scene_reader = std::make_shared<SceneReader>(engine_context);
    }

    std::string Runner::getScenePath() const {
        return scene_manager->scene != nullptr ? scene_manager->scene->path : "";
    }

    void Runner::loadScene(const std::string &scene_path) {
        assert(!scene_path.empty());

        scene_manager->scene = scene_reader->readScene(scene_path, renderer->getMaterials());
        renderer->loadScene(scene_manager->scene);
        //SceneWriter writer;
        //writer.writeScene(PathUtil::getFileName(scene_manager->getSceneInformation().path), scene_manager->scene);
    }

    void Runner::renderScene() {
        drawFrame();
    }

    void Runner::drawFrame() {
        renderer->waitForNextFrameStart();

        const int32_t swapchain_image_idx = renderer->aquireNextSwapchainImage();
        if (swapchain_image_idx < 0)
            return;

        renderer->resetCurrFrame();

        VkExtent2D extent = renderer->getSwapchainExtent();
        scene_manager->scene->update(extent.width, extent.height);
        renderer->update();

        VkCommandBuffer command_buffer = renderer->getNewCommandBuffer();
        renderer->recordBeginCommandBuffer(command_buffer);
        renderer->recordCommandBuffer(command_buffer, swapchain_image_idx, true);
        gui_manager->recordGuiCommands(command_buffer, swapchain_image_idx);
        renderer->recordEndCommandBuffer(command_buffer);
        if (renderer->submitCommands(true, swapchain_image_idx)) {
            renderer->waitForIdle();
            renderer->refreshAfterResize();
            gui_manager->updateWindows();
        }
    }


} // RtEngine