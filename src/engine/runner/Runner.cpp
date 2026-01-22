#include "../../../include/engine/runner/Runner.hpp"

#include "UpdateFlags.hpp"

namespace RtEngine {
    Runner::Runner(std::shared_ptr<EngineContext> engine_context,
        const std::shared_ptr<GuiManager> &gui_manager, const std::shared_ptr<SceneManager> &scene_manager)
        : engine_context(engine_context), renderer(engine_context->renderer),
        gui_manager(gui_manager), scene_manager(scene_manager) {

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
        update_flags |= MATERIAL_UPDATE | STATIC_GEOMETRY_UPDATE;

        //SceneWriter writer;
        //writer.writeScene(PathUtil::getFileName(scene_manager->getSceneInformation().path), scene_manager->scene);
    }

    void Runner::renderScene() {
        scene_manager->getCurrentScene()->update();
        std::shared_ptr<DrawContext> draw_context = createMainDrawContext();
        if (draw_context->targets.size() < 1)
            return;
        drawFrame(draw_context);
    }

    void Runner::drawFrame(const std::shared_ptr<DrawContext>& draw_context) {
        renderer->waitForNextFrameStart();

        const int32_t swapchain_image_idx = renderer->aquireNextSwapchainImage();
        if (swapchain_image_idx < 0) {
            handle_resize();
            return;
        }

        renderer->resetCurrFrameFence();

        VkCommandBuffer cmd = renderer->getNewCommandBuffer();
        std::shared_ptr<RenderTarget> target = draw_context->targets[0]; // TODO handle multiple

        prepareFrame(cmd, draw_context);

        renderer->updateRenderTarget(target);
        renderer->recordCommandBuffer(cmd, target, swapchain_image_idx, true);
        gui_manager->recordGuiCommands(cmd, swapchain_image_idx);

        finishFrame(cmd, draw_context, static_cast<uint32_t>(swapchain_image_idx), true);
    }

    void Runner::prepareFrame(VkCommandBuffer cmd, const std::shared_ptr<DrawContext> &draw_context) {
        renderer->updateSceneRepresentation(draw_context, update_flags);

        if (update_flags > 0) {
            for (const auto& target : draw_context->targets) {
                target->resetAccumulatedFrames();
            }
        }
        update_flags = 0;

        renderer->recordBeginCommandBuffer(cmd);
    }

    void Runner::finishFrame(VkCommandBuffer cmd, const std::shared_ptr<DrawContext> &draw_context, uint32_t swapchain_image_idx, bool present) const {
        renderer->recordEndCommandBuffer(cmd);
        if (renderer->submitCommands(present, swapchain_image_idx)) {
            handle_resize();
        }
        renderer->nextFrame();
        draw_context->nextFrame();
    }

    void Runner::handle_resize() const {
        renderer->waitForIdle();
        engine_context->swapchain_manager->recreate();
        gui_manager->updateWindows();
    }

    std::shared_ptr<DrawContext> Runner::createMainDrawContext() const {
        auto draw_context = std::make_shared<DrawContext>();
        scene_manager->getCurrentScene()->fillDrawContext(draw_context);
        return draw_context;
    }

    void Runner::setUpdateFlags(uint32_t new_flags) {
        update_flags |= new_flags;
    }

    bool Runner::isRunning() const {
        return running;
    }
} // RtEngine