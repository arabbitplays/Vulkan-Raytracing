//
// Created by oschdi on 28.01.26.
//

#include "../../../../include/engine/rendering/renderer/Renderer.hpp"

#include "MeshAsset.hpp"

namespace RtEngine {
    Renderer::Renderer(std::shared_ptr<VulkanContext> vulkan_context, const uint32_t max_frames_in_flight)
        : vulkan_context(vulkan_context), max_frames_in_flight(max_frames_in_flight) {
    }

    void Renderer::init() {
        createSyncObjects();
        createCommandBuffers();
    }

    void Renderer::createCommandBuffers() {
        command_buffers.resize(max_frames_in_flight);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = vulkan_context->command_manager->commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t) command_buffers.size();

        if (vkAllocateCommandBuffers(vulkan_context->device_manager->getDevice(), &allocInfo, command_buffers.data()) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command bufer!");
            }
    }

    void Renderer::createSyncObjects() {
        in_flight_fences.resize(max_frames_in_flight);

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < max_frames_in_flight; i++) {
            if (vkCreateFence(vulkan_context->device_manager->getDevice(), &fenceInfo, nullptr, &in_flight_fences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create sync objects");
            }

            deletion_queue.pushFunction([&, i]() {
                vkDestroyFence(vulkan_context->device_manager->getDevice(), in_flight_fences[i], nullptr);
            });
        }
    }

    void Renderer::waitForNextFrameStart() {
        vkWaitForFences(vulkan_context->device_manager->getDevice(), 1, &in_flight_fences[current_frame], VK_TRUE,
                        UINT64_MAX);
        vkResetFences(vulkan_context->device_manager->getDevice(), 1, &in_flight_fences[current_frame]);
    }

    VkCommandBuffer Renderer::getNewCommandBuffer() {
        vkResetCommandBuffer(command_buffers[current_frame], 0);
        return command_buffers[current_frame];
    }

    void Renderer::nextFrame() {
        current_frame = (current_frame + 1) % max_frames_in_flight;
    }
} // RtEngine