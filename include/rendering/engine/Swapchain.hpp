#ifndef SWAPCHAIN_HPP
#define SWAPCHAIN_HPP

#include <vector>
#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>
#include "RessourceBuilder.hpp"

namespace RtEngine {
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class Swapchain {
public:
    Swapchain() = default;
    Swapchain(VkDevice device, VkPhysicalDevice physical_device, GLFWwindow* window, VkSurfaceKHR surface, RessourceBuilder resource_builder)
            : device(device), physical_device(physical_device), window(window), surface(surface), ressource_builder(resource_builder) {
        createSwapChain();
    };
    static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
    void recreate();
    void destroy();

    VkDevice device;
    VkPhysicalDevice physical_device;
    GLFWwindow* window;
    VkSurfaceKHR surface;
    RessourceBuilder ressource_builder;

    VkSwapchainKHR handle;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    VkFormat imageFormat;
    VkExtent2D extent;

private:
    void createSwapChain();
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtend(const VkSurfaceCapabilitiesKHR& capabilities);
    void createImageViews();
};



}
#endif //SWAPCHAIN_HPP
