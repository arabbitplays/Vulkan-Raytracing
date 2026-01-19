//
// Created by oschdi on 19.01.26.
//

#include "../../include/engine/SwapchainManager.hpp"

namespace RtEngine {
    void SwapchainManager::addRecreateCallback(std::function<void(uint32_t, uint32_t)> func) {
        recreate_callbacks.push_back(func);
    }

    void SwapchainManager::recreate(uint32_t new_width, uint32_t new_height) {
        for (const auto& callback : recreate_callbacks) {
            callback(new_width, new_height);
        }
    }
} // RtEngine