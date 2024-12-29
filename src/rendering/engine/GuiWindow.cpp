//
// Created by oschdi on 12/29/24.
//

#include "GuiWindow.hpp"

#include <CommandManager.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <RenderPassBuilder.hpp>
#include <stdexcept>

// only used as imgui callback
void checkVulkanResult(VkResult err)
{
    if (err == 0)
        return;
    throw std::runtime_error("Error: VkResult = " + err);
}

GuiWindow::GuiWindow(VkDevice device, VkPhysicalDevice physical_device, GLFWwindow* window, VkInstance instance,
              DescriptorAllocator descriptor_allocator, std::shared_ptr<SwapChain> swapchain,
              uint32_t grafics_queue_family, VkQueue grafics_queue) : device(device), swapchain(swapchain) {

    createRenderPass(swapchain->swapChainImageFormat);
    createFrameBuffers();
    createDescriptorPool(descriptor_allocator);

    deletion_queue.pushFunction([&]() {
        vkDestroyRenderPass(this->device, render_pass, nullptr);
        vkDestroyDescriptorPool(this->device, descriptor_pool, nullptr);
    });

    initImGui(physical_device, window, instance, grafics_queue_family, grafics_queue);
}

void GuiWindow::createRenderPass(VkFormat image_format) {
    RenderPassBuilder renderPassBuilder;
    renderPassBuilder.setColorAttachmentFormat(image_format);
    render_pass = renderPassBuilder.createRenderPass(device);
}

void GuiWindow::createFrameBuffers() {
    frame_buffers.resize(swapchain->swapChainImageViews.size());
    for (size_t i = 0; i < frame_buffers.size(); i++) {
        std::array<VkImageView, 1> attachments {
            swapchain->swapChainImageViews[i],
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = render_pass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapchain->swapChainExtent.width;
        framebufferInfo.height = swapchain->swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &frame_buffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void GuiWindow::createDescriptorPool(DescriptorAllocator descriptor_allocator) {
    std::vector<VkDescriptorPoolSize> pool_sizes =
        {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
    };
    descriptor_pool = descriptor_allocator.createPool(device, pool_sizes, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
}


void GuiWindow::initImGui(VkPhysicalDevice physical_device, GLFWwindow* window, VkInstance instance,
        uint32_t graphics_queue_family, VkQueue graphics_queue) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = instance;
    initInfo.PhysicalDevice = physical_device;
    initInfo.Device = device;
    initInfo.QueueFamily = graphics_queue_family;
    initInfo.Queue = graphics_queue;
    initInfo.PipelineCache = VK_NULL_HANDLE;
    initInfo.DescriptorPool = descriptor_pool;
    initInfo.RenderPass = render_pass;
    initInfo.Subpass = 0;
    initInfo.MinImageCount = minImageCount;
    initInfo.ImageCount = static_cast<uint32_t>(swapchain->swapChainImages.size());
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.Allocator = nullptr;
    initInfo.CheckVkResultFn = checkVulkanResult;
    ImGui_ImplVulkan_Init(&initInfo);
}

void GuiWindow::updateWindow(std::shared_ptr<SwapChain> swapchain) {
    this->swapchain = swapchain;
    createFrameBuffers();
}


void GuiWindow::destroy() {
    deletion_queue.flush();
}


