//
// Created by oschdi on 12/29/24.
//

#ifndef SWAPCHAIN_HPP
#define SWAPCHAIN_HPP

#include <vector>
#include <vulkan/vulkan_core.h>

class SwapChain {
public:
    SwapChain() = default;

    VkSwapchainKHR handle;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
};



#endif //SWAPCHAIN_HPP
