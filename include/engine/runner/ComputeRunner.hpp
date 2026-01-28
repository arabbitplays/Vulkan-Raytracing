#ifndef VULKAN_RAYTRACING_COMPUTERUNNER_HPP
#define VULKAN_RAYTRACING_COMPUTERUNNER_HPP
#include "Runner.hpp"

namespace RtEngine {
    class ComputeRunner final : public Runner {
    public:
        ComputeRunner(const std::shared_ptr<EngineContext> &engine_context,
            const std::shared_ptr<SceneManager> &scene_manager);

        void renderScene() override;

    protected:
        void drawFrame(const std::shared_ptr<DrawContext> &draw_context) override;

        void prepareFrame(VkCommandBuffer cmd, const std::shared_ptr<DrawContext> &draw_context) override;

        void finishFrame(VkCommandBuffer cmd, const std::shared_ptr<DrawContext> &draw_context,
            uint32_t swapchain_image_idx, bool present) const override;

        std::shared_ptr<ComputeRenderer> compute_renderer;

        const std::string INPUT_DIR = "../resources/compute_in";
        const std::string OUTPUT_DIR = "../resources/compute_out";

    };
} // RtEngine

#endif //VULKAN_RAYTRACING_COMPUTERUNNER_HPP