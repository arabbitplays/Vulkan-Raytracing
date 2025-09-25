#ifndef VULKAN_RAYTRACING_COMMANDBUFFERUTIL_HPP
#define VULKAN_RAYTRACING_COMMANDBUFFERUTIL_HPP
#include "CommandManager.hpp"

namespace RtEngine {
    class CommandBufferUtil {
    public:
        CommandBufferUtil() = delete;
        ~CommandBufferUtil() = delete;

        static void recordBeginCommandBuffer(const VkCommandBuffer command_buffer) {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

            if (vkBeginCommandBuffer(command_buffer, &beginInfo) != VK_SUCCESS) {
                throw std::runtime_error("failed to begin record command buffer!");
            }
        }

        static void recordEndCommandBuffer(const VkCommandBuffer command_buffer) {
            if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
                throw std::runtime_error("failed to record command buffer!");
            }
        }

        static void recordMemoryBarrier(const VkCommandBuffer command_buffer,
            const VkPipelineStageFlags2 src_stage, const VkAccessFlags2 src_access,
            const VkPipelineStageFlags2 dst_stage, const VkAccessFlags2 dst_access) {

            VkMemoryBarrier2 barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
            barrier.srcStageMask = src_stage;
            barrier.srcAccessMask = src_access;
            barrier.dstStageMask = dst_stage;
            barrier.dstAccessMask = dst_access;

            VkDependencyInfo dep_info{};
            dep_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
            dep_info.memoryBarrierCount = 1;
            dep_info.pMemoryBarriers = &barrier;

            vkCmdPipelineBarrier2(command_buffer, &dep_info);
        }
    };
}



#endif //VULKAN_RAYTRACING_COMMANDBUFFERUTIL_HPP