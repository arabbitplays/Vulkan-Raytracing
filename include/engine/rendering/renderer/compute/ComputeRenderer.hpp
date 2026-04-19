#ifndef VULKAN_RAYTRACING_COMPUTERENDERER_HPP
#define VULKAN_RAYTRACING_COMPUTERENDERER_HPP
#include <memory>

#include "ComputePipeline.hpp"
#include "DescriptorLayoutBuilder.hpp"
#include "../Renderer.hpp"
#include "../../RenderTarget.hpp"
#include "VulkanContext.hpp"

namespace RtEngine {
    class ComputeRenderer : public Renderer {
    public:
        ComputeRenderer(const std::shared_ptr<VulkanContext>& vulkan_context, const uint32_t max_frames_in_flight = 1);

        void init() override;

        void writeRenderTarget(const std::shared_ptr<RenderTarget> &target) override = 0;
        void writeResources(const std::shared_ptr<DrawContext> &draw_context, UpdateFlagsHandle update_flags) override = 0;

        void recordCommandBuffer(VkCommandBuffer commandBuffer, std::shared_ptr<RenderTarget> target, uint32_t swapchain_image_idx);
        void submitCommandBuffer(VkCommandBuffer &command_buffer);

        void cleanup();
    protected:
        void createPipeline();
        virtual void initDescriptorLayout(DescriptorLayoutBuilder &layout_builder) = 0;
        virtual VkShaderModule createShaderModule() = 0;

        virtual void recordDispatch(VkCommandBuffer command_buffer, std::shared_ptr<RenderTarget> &target) = 0;

        std::shared_ptr<ComputePipeline> pipeline;
        VkDescriptorSetLayout descriptor_layout;
        VkDescriptorSet descriptor_set;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_COMPUTERENDERER_HPP