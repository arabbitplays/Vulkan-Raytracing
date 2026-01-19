//
// Created by oschdi on 19.01.26.
//

#ifndef VULKAN_RAYTRACING_SWAPCHAINMANAGER_HPP
#define VULKAN_RAYTRACING_SWAPCHAINMANAGER_HPP
#include <vector>
#include <Swapchain.hpp>

namespace RtEngine {
    class SwapchainManager {
    public:
        SwapchainManager() = default;

        void addRecreateCallback(std::function<void(uint32_t, uint32_t)> func);

        void recreate(uint32_t new_width, uint32_t new_height);

    private:
        std::vector<std::function<void(uint32_t, uint32_t)>> recreate_callbacks;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_SWAPCHAINMANAGER_HPP