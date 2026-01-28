//
// Created by oschdi on 28.01.26.
//

#ifndef VULKAN_RAYTRACING_RENDERER_HPP
#define VULKAN_RAYTRACING_RENDERER_HPP
#include "RenderTarget.hpp"
#include "VulkanContext.hpp"

namespace RtEngine {
    class Renderer {
    public:
        Renderer(std::shared_ptr<VulkanContext> vulkan_context, const uint32_t max_frames_in_flight);

        virtual void init();

        virtual void updateRenderTarget(const std::shared_ptr<RenderTarget> &target) = 0;

        void waitForNextFrameStart();

        VkCommandBuffer getNewCommandBuffer();

        void nextFrame();

    protected:
        void createCommandBuffers();
        virtual void createSyncObjects();

        std::shared_ptr<VulkanContext> vulkan_context;
        uint32_t max_frames_in_flight, current_frame = 0;

        std::vector<VkCommandBuffer> command_buffers;
        std::vector<VkFence> in_flight_fences;

        DeletionQueue deletion_queue;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_RENDERER_HPP