#ifndef VULKAN_RAYTRACING_COMPUTERENDERER_HPP
#define VULKAN_RAYTRACING_COMPUTERENDERER_HPP
#include <memory>

#include "RenderTarget.hpp"
#include "VulkanContext.hpp"

namespace RtEngine {
    class ComputeRenderer {
    public:
        ComputeRenderer(const std::shared_ptr<VulkanContext>& vulkan_context);

        virtual void recordCommandBuffer(VkCommandBuffer commandBuffer, std::shared_ptr<RenderTarget> target, uint32_t swapchain_image_idx);
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_COMPUTERENDERER_HPP