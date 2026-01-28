#ifndef VULKAN_RAYTRACING_COMPUTERENDERER_HPP
#define VULKAN_RAYTRACING_COMPUTERENDERER_HPP
#include <memory>

#include "ComputePipeline.hpp"
#include "DescriptorLayoutBuilder.hpp"
#include "Renderer.hpp"
#include "../RenderTarget.hpp"
#include "VulkanContext.hpp"

namespace RtEngine {
    class ComputeRenderer : public Renderer {
    public:
        ComputeRenderer(const std::shared_ptr<VulkanContext>& vulkan_context, const uint32_t max_frames_in_flight = 1);

        void updateRenderTarget(const std::shared_ptr<RenderTarget> &target) override;
        void updateResources(const AllocatedImage &src_image);

        void recordCommandBuffer(VkCommandBuffer commandBuffer, std::shared_ptr<RenderTarget> target, uint32_t swapchain_image_idx);

        void submitCommandBuffer(VkCommandBuffer &command_buffer);

        void cleanup();
    private:
        void createPipeline();
        void initDescriptorLayout(DescriptorLayoutBuilder &layout_builder);

        std::shared_ptr<ComputePipeline> pipeline;
        VkDescriptorSetLayout descriptor_layout;
        VkDescriptorSet descriptor_set;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_COMPUTERENDERER_HPP