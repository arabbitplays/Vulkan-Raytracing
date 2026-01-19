//
// Created by oschdi on 19.01.26.
//

#include "../../include/engine/SwapchainManager.hpp"

#include <cassert>

namespace RtEngine {
    SwapchainManager::SwapchainManager(const std::shared_ptr<Swapchain> &swapchain) : swapchain(swapchain) {
        assert(swapchain != nullptr);
    }

    void SwapchainManager::addRecreateCallback(const std::function<void(uint32_t, uint32_t)> &func) {
        recreate_callbacks.push_back(func);
    }

    void SwapchainManager::recreate() const {
        swapchain->recreate();
        VkExtent2D new_extent = swapchain->extent;
        for (const auto& callback : recreate_callbacks) {
            callback(new_extent.width, new_extent.height);
        }
    }

    VkExtent2D SwapchainManager::getSwapchainExtent() const {
        return swapchain->extent;
    }
} // RtEngine

