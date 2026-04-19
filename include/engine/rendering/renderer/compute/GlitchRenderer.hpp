#ifndef VULKAN_RAYTRACING_GLITCHRENDERER_HPP
#define VULKAN_RAYTRACING_GLITCHRENDERER_HPP
#include "ComputeRenderer.hpp"

namespace RtEngine {
    class GlitchRenderer : public ComputeRenderer {
    public:
        GlitchRenderer(const std::shared_ptr<VulkanContext>& vulkan_context, const uint32_t max_frames_in_flight = 1);

        void writeRenderTarget(const std::shared_ptr<RenderTargetRepository> &target) override;
        void writeResources(const std::shared_ptr<DrawContext> &draw_context, UpdateFlagsHandle update_flags) override;

    protected:
        void initDescriptorLayout(DescriptorLayoutBuilder &layout_builder) override;
        void recordDispatch(VkCommandBuffer command_buffer, std::shared_ptr<RenderTargetRepository> &target) override;

        VkShaderModule createShaderModule() override;

        const std::string INPUT_DIR = "../resources/compute_in";
    };
}
#endif //VULKAN_RAYTRACING_GLITCHRENDERER_HPP
