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
        explicit SwapchainManager(const std::shared_ptr<Swapchain> &swapchain);

        void addRecreateCallback(const std::function<void(uint32_t, uint32_t)> &func);

        void recreate() const;

        VkExtent2D getSwapchainExtent() const;

    private:
        std::shared_ptr<Swapchain> swapchain;
        std::vector<std::function<void(uint32_t, uint32_t)>> recreate_callbacks;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_SWAPCHAINMANAGER_HPP