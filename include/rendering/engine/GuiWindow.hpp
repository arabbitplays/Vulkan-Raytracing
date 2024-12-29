//
// Created by oschdi on 12/29/24.
//

#ifndef GUIWINDOW_HPP
#define GUIWINDOW_HPP

#include <vulkan/vulkan_core.h>
#include "DeletionQueue.hpp"
#include <DescriptorAllocator.hpp>
#include <GLFW/glfw3.h>
#include <Swapchain.hpp>
#include <bits/shared_ptr.h>

class GuiWindow {
public:
    GuiWindow() = default;
    GuiWindow(VkDevice device, VkPhysicalDevice physical_device, GLFWwindow* window, VkInstance instance,
              DescriptorAllocator descriptor_allocator, std::shared_ptr<Swapchain> swapchain,
              uint32_t grafics_queue_family, VkQueue grafics_queue);
    void updateWindow(std::shared_ptr<Swapchain> swapchain);
    void destroy();

    VkRenderPass render_pass;
    std::vector<VkFramebuffer> frame_buffers;

private:
    void createRenderPass(VkFormat image_format);
    void createFrameBuffers();
    void createDescriptorPool(DescriptorAllocator descriptor_allocator);
    void initImGui(VkPhysicalDevice physical_device, GLFWwindow* window, VkInstance instance,
        uint32_t graphics_queue_family, VkQueue graphics_queue);

    VkDevice device;
    std::shared_ptr<Swapchain> swapchain;

    uint32_t minImageCount = 2;
    VkDescriptorPool descriptor_pool;

    DeletionQueue deletion_queue{};
};



#endif //GUIWINDOW_HPP
