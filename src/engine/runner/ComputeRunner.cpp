#include "../../../include/engine/runner/ComputeRunner.hpp"

namespace RtEngine {
    ComputeRunner::ComputeRunner(const std::shared_ptr<EngineContext> &engine_context, const std::shared_ptr<SceneManager> &scene_manager)
        : Runner(engine_context, scene_manager),
          compute_renderer(engine_context->rendering_manager->getGlitchRenderer()) {
    }

    void ComputeRunner::renderScene() {
        std::shared_ptr<DrawContext> draw_context = std::make_shared<DrawContext>();
        draw_context->targets.push_back(engine_context->rendering_manager->createRenderTarget(1000, 1000));
        compute_renderer->updateRenderTarget(draw_context->targets[0]);
        Runner::renderScene();
    }

    void ComputeRunner::drawFrame(const std::shared_ptr<DrawContext> &draw_context) {
        compute_renderer->waitForNextFrameStart();

        VkCommandBuffer cmd = compute_renderer->getNewCommandBuffer();
        std::shared_ptr<RenderTarget> target = draw_context->targets[0];

        prepareFrame(cmd, draw_context);

        compute_renderer->recordCommandBuffer(cmd, target, 0);

        finishFrame(cmd, draw_context, 0, false);
    }

    void ComputeRunner::prepareFrame(VkCommandBuffer cmd, const std::shared_ptr<DrawContext> &draw_context) {
        compute_renderer->updateResources(draw_context->targets[0]->getCurrentTargetImage());
        engine_context->rendering_manager->recordBeginCommandBuffer(cmd);
    }

    void ComputeRunner::finishFrame(VkCommandBuffer cmd, const std::shared_ptr<DrawContext> &draw_context,
        uint32_t swapchain_image_idx, bool present) const {
        engine_context->rendering_manager->recordEndCommandBuffer(cmd);
        compute_renderer->submitCommandBuffer(cmd);
        raytracing_renderer->nextFrame();
    }
} // RtEngine