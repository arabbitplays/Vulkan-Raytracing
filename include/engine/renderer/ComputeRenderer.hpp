#ifndef VULKAN_RAYTRACING_COMPUTERENDERER_HPP
#define VULKAN_RAYTRACING_COMPUTERENDERER_HPP
#include <memory>

#include "ComputePipeline.hpp"
#include "DescriptorLayoutBuilder.hpp"
#include "RenderTarget.hpp"
#include "VulkanContext.hpp"

namespace RtEngine {
    class ComputeRenderer {
    public:
        ComputeRenderer(const std::shared_ptr<VulkanContext>& vulkan_context);

        void recordCommandBuffer(VkCommandBuffer commandBuffer, std::shared_ptr<RenderTarget> target, uint32_t swapchain_image_idx);

        void cleanup();
    private:
        void createPipeline();
        void initDescriptorLayout(DescriptorLayoutBuilder &layout_builder);

        std::shared_ptr<VulkanContext> vulkan_context;
        std::shared_ptr<ComputePipeline> pipeline;
        VkDescriptorSetLayout descriptor_layout;
        VkDescriptorSet descriptor_set;

        DeletionQueue deletion_queue;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_COMPUTERENDERER_HPP