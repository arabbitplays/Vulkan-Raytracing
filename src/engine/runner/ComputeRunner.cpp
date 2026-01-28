#include "../../../include/engine/runner/ComputeRunner.hpp"

#include <filesystem>

namespace RtEngine {
    ComputeRunner::ComputeRunner(const std::shared_ptr<EngineContext> &engine_context, const std::shared_ptr<SceneManager> &scene_manager)
        : Runner(engine_context, scene_manager),
          compute_renderer(engine_context->rendering_manager->getGlitchRenderer()) {
    }

    void ComputeRunner::renderScene() {
        std::shared_ptr<DrawContext> draw_context = std::make_shared<DrawContext>();
        draw_context->targets.resize(1);

        for (const auto& entry : std::filesystem::directory_iterator(INPUT_DIR)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            std::filesystem::path file_name = entry.path().filename();
            if (file_name.extension() != ".png") {
                continue;
            }
            AllocatedImage input_img = engine_context->rendering_manager->getVulkanContext()->resource_builder->loadImage(entry.path().string(), VK_IMAGE_LAYOUT_GENERAL);
            compute_renderer->updateResources(input_img);

            VkExtent3D extent = input_img.imageExtent;
            std::shared_ptr<RenderTarget> target = engine_context->rendering_manager->createRenderTarget(extent.width, extent.height);
            draw_context->targets[0] = target;
            compute_renderer->updateRenderTarget(target);

            drawFrame(draw_context);
            raytracing_renderer->waitForIdle();
            raytracing_renderer->outputRenderingTarget(target, OUTPUT_DIR + "/" + file_name.string());

            engine_context->rendering_manager->getVulkanContext()->resource_builder->destroyImage(input_img);
            target->destroy();
        }

        running = false;
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
        engine_context->rendering_manager->recordBeginCommandBuffer(cmd);
    }

    void ComputeRunner::finishFrame(VkCommandBuffer cmd, const std::shared_ptr<DrawContext> &draw_context,
        uint32_t swapchain_image_idx, bool present) const {
        engine_context->rendering_manager->recordEndCommandBuffer(cmd);
        compute_renderer->submitCommandBuffer(cmd);
        raytracing_renderer->nextFrame();
    }
} // RtEngine