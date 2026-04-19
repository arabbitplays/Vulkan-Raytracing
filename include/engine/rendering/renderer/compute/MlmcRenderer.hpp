#ifndef VULKAN_RAYTRACING_MCMPRENDERER_HPP
#define VULKAN_RAYTRACING_MCMPRENDERER_HPP
#include "ComputeRenderer.hpp"

namespace RtEngine {
    class MlmcRenderer : public ComputeRenderer {
    public:
        MlmcRenderer(const std::shared_ptr<VulkanContext> &vulkan_context, uint32_t max_frames_in_flight)
            : ComputeRenderer(vulkan_context, max_frames_in_flight) {
        }

        void writeRenderTarget(const std::shared_ptr<RenderTargetRepository> &target) override;

        void writeResources(const std::shared_ptr<DrawContext> &draw_context, UpdateFlagsHandle update_flags) override;

    protected:
        void initDescriptorLayout(DescriptorLayoutBuilder &layout_builder) override;

        VkShaderModule createShaderModule() override;

        void recordDispatch(VkCommandBuffer command_buffer, std::shared_ptr<RenderTargetRepository> &target) override;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_MCMPRENDERER_HPP