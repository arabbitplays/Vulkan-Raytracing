//
// Created by oschdi on 12/29/24.
//

#ifndef GUIMANAGER_HPP
#define GUIMANAGER_HPP

#include <vulkan/vulkan_core.h>
#include "DeletionQueue.hpp"
#include <DescriptorAllocator.hpp>
#include <imgui.h>
#include <GLFW/glfw3.h>
#include <Swapchain.hpp>
#include <OptionsWindow.hpp>
#include <bits/shared_ptr.h>

class GuiManager {
public:
    GuiManager() = default;
    GuiManager(VkDevice device, VkPhysicalDevice physical_device, GLFWwindow* window, VkInstance instance,
              DescriptorAllocator descriptor_allocator, std::shared_ptr<Swapchain> swapchain,
              uint32_t graphics_queue_family, VkQueue graphics_queue);

    void addOptionsWindow(std::shared_ptr<OptionsWindow> window);
    void addPropertiesToOptions(std::shared_ptr<Properties> properties);
    void updateWindows(std::shared_ptr<Swapchain> swapchain);
    void recordGuiCommands(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void destroy();

    VkRenderPass render_pass;
    std::vector<VkFramebuffer> frame_buffers;

private:
    void createRenderPass(VkFormat image_format);
    void createFrameBuffers();
    void createDescriptorPool(DescriptorAllocator descriptor_allocator);
    void initImGui(VkPhysicalDevice physical_device, GLFWwindow* window, VkInstance instance,
        uint32_t graphics_queue_family, VkQueue graphics_queue);
    void shutdownImGui();

    VkDevice device;
    std::shared_ptr<Swapchain> swapchain;

    std::shared_ptr<OptionsWindow> options_window;
	std::vector<std::shared_ptr<GuiWindow>> gui_windows{};

    uint32_t minImageCount = 2;
    VkDescriptorPool descriptor_pool;

    DeletionQueue deletion_queue{};

    bool show_demo_window = true;
    bool show_main_window = true;
};



#endif //GUIWINDOW_HPP