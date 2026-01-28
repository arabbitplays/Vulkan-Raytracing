#include "../../../include/engine/runner/Runner.hpp"

#include "PathUtil.hpp"
#include "SceneWriter.hpp"
#include "UpdateFlagValue.hpp"

namespace RtEngine {
    Runner::Runner(std::shared_ptr<EngineContext> engine_context, const std::shared_ptr<SceneManager> &scene_manager)
        : engine_context(engine_context), scene_manager(scene_manager),
          raytracing_renderer(engine_context->rendering_manager->getRaytracingRenderer()),
          gui_renderer(engine_context->rendering_manager->getGuiRenderer()) {

        scene_reader = std::make_shared<SceneReader>(engine_context);
        update_flags = std::make_shared<UpdateFlags>();
    }

    std::string Runner::getScenePath() const {
        return scene_manager->getCurrentScene() != nullptr ? scene_manager->getCurrentScene()->path : "";
    }

    void Runner::loadScene(const std::string &scene_path) {
        assert(!scene_path.empty());

        std::shared_ptr<Scene> old_scene = scene_manager->getCurrentScene(); // hold until it can be safely destroyed
        std::shared_ptr<Scene> new_scene = scene_reader->readScene(scene_path, raytracing_renderer->getMaterials());
        scene_manager->setScene(new_scene);

        raytracing_renderer->waitForIdle();
        if (old_scene != nullptr) {
            old_scene->destroy();
        }
        new_scene->start();
        raytracing_renderer->loadScene(new_scene);

        SceneWriter writer;
        writer.writeScene(PathUtil::getFileName(scene_path), new_scene);
    }

    void Runner::renderScene() {
        if (update_flags->checkFlag(SCENE_UPDATE)) {
            loadScene(scene_manager->getScenePath(scene_name));
        }

        scene_manager->getCurrentScene()->update();
        std::shared_ptr<DrawContext> draw_context = createMainDrawContext();
        if (draw_context->targets.size() < 1)
            return;
        drawFrame(draw_context);
    }

    void Runner::setUpdateFlags(const UpdateFlagsHandle &new_flags) const {
        update_flags->setFlags(new_flags);
    }

    bool Runner::isRunning() const {
        return running;
    }

    void Runner::initProperties(const std::shared_ptr<IProperties> &config, const UpdateFlagsHandle& update_flags) {
        if (config->startChild("runner")) {
            if (config->addSelection("scene_name", &scene_name, scene_manager->getSceneNames())) {
                update_flags->setFlag(SCENE_UPDATE);
            }
            config->endChild();
        }
    }

    void Runner::drawFrame(const std::shared_ptr<DrawContext>& draw_context) {
        raytracing_renderer->waitForNextFrameStart();

        const int32_t swapchain_image_idx = raytracing_renderer->aquireNextSwapchainImage();
        if (swapchain_image_idx < 0) {
            handle_resize();
            return;
        }

        VkCommandBuffer cmd = raytracing_renderer->getNewCommandBuffer();
        std::shared_ptr<RenderTarget> target = draw_context->targets[0]; // TODO handle multiple

        prepareFrame(cmd, draw_context);

        raytracing_renderer->updateRenderTarget(target);
        raytracing_renderer->recordCommandBuffer(cmd, target, swapchain_image_idx, true);
        gui_renderer->recordGuiCommands(cmd, swapchain_image_idx);

        finishFrame(cmd, draw_context, static_cast<uint32_t>(swapchain_image_idx), true);
    }

    void Runner::prepareFrame(VkCommandBuffer cmd, const std::shared_ptr<DrawContext> &draw_context) {
        raytracing_renderer->updateSceneRepresentation(draw_context, update_flags);

        if (update_flags->checkFlag(TARGET_RESET)) {
            for (const auto& target : draw_context->targets) {
                target->resetAccumulatedFrames();
            }
        }
        update_flags->resetFlags();

        engine_context->rendering_manager->recordBeginCommandBuffer(cmd);
    }

    void Runner::finishFrame(VkCommandBuffer cmd, const std::shared_ptr<DrawContext> &draw_context, uint32_t swapchain_image_idx, bool present) const {
        engine_context->rendering_manager->recordEndCommandBuffer(cmd);
        if (raytracing_renderer->submitCommands(present, swapchain_image_idx)) {
            handle_resize();
        }
        raytracing_renderer->nextFrame();
        draw_context->nextFrame();
    }

    void Runner::handle_resize() const {
        raytracing_renderer->waitForIdle();
        engine_context->swapchain_manager->recreate();
        gui_renderer->recreateFramebuffer();
    }

    std::shared_ptr<DrawContext> Runner::createMainDrawContext() const {
        auto draw_context = std::make_shared<DrawContext>();
        scene_manager->getCurrentScene()->fillDrawContext(draw_context);
        return draw_context;
    }
} // RtEngine