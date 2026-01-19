//
// Created by oschdi on 18.01.26.
//

#include "../../../include/engine/runner/Runner.hpp"

namespace RtEngine {
    Runner::Runner(std::shared_ptr<EngineContext> engine_context, std::shared_ptr<GuiManager> gui_manager, std::shared_ptr<SceneManager> scene_manager) : engine_context(engine_context), renderer(engine_context->renderer), gui_manager(gui_manager), scene_manager(scene_manager) {
        scene_reader = std::make_shared<SceneReader>(engine_context);
    }

    std::string Runner::getScenePath() const {
        return scene_manager->getCurrentScene() != nullptr ? scene_manager->getCurrentScene()->path : "";
    }

    void Runner::loadScene(const std::string &scene_path) {
        assert(!scene_path.empty());

        std::shared_ptr<Scene> old_scene = scene_manager->getCurrentScene(); // hold until it can be safely destroyed
        std::shared_ptr<Scene> new_scene = scene_reader->readScene(scene_path, renderer->getMaterials());
        scene_manager->setScene(new_scene);

        renderer->waitForIdle();
        if (old_scene != nullptr) {
            old_scene->destroy();
        }
        new_scene->start();
        renderer->loadScene(new_scene);

        //SceneWriter writer;
        //writer.writeScene(PathUtil::getFileName(scene_manager->getSceneInformation().path), scene_manager->scene);
    }

    void Runner::renderScene() {
        scene_manager->getCurrentScene()->update();
        std::shared_ptr<DrawContext> draw_context = createMainDrawContext();
        drawFrame(draw_context);
    }

    void Runner::drawFrame(std::shared_ptr<DrawContext> draw_context) {
        renderer->waitForNextFrameStart();

        const int32_t swapchain_image_idx = renderer->aquireNextSwapchainImage();
        if (swapchain_image_idx < 0)
            return;

        renderer->resetCurrFrame();

        renderer->update(draw_context);

        if (draw_context->targets.size() < 0)
            return;
        std::shared_ptr<RenderTarget> target = draw_context->targets[0]; // TODO handle multiple

        VkCommandBuffer command_buffer = renderer->getNewCommandBuffer();
        renderer->recordBeginCommandBuffer(command_buffer);
        renderer->recordCommandBuffer(command_buffer, target, swapchain_image_idx, true);
        gui_manager->recordGuiCommands(command_buffer, swapchain_image_idx);
        renderer->recordEndCommandBuffer(command_buffer);
        if (renderer->submitCommands(true, swapchain_image_idx)) {
            renderer->waitForIdle();
            renderer->refreshAfterResize();
            VkExtent2D extent = renderer->getSwapchainExtent();
            engine_context->swapchain_manager->recreate(extent.width, extent.height);
            gui_manager->updateWindows();
        }
        renderer->nextFrame();
        draw_context->nextFrame();
    }

    std::shared_ptr<DrawContext> Runner::createMainDrawContext() {
        auto draw_context = std::make_shared<DrawContext>();
        scene_manager->getCurrentScene()->fillDrawContext(draw_context);
        return draw_context;
    }


} // RtEngine