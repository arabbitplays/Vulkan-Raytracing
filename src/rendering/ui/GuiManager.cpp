//
// Created by oschdi on 12/29/24.
//

#include "GuiManager.hpp"

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

GuiManager::GuiManager(VkDevice device, VkPhysicalDevice physical_device, GLFWwindow* window, VkInstance instance,
              DescriptorAllocator descriptor_allocator, std::shared_ptr<Swapchain> swapchain,
              uint32_t grafics_queue_family, VkQueue grafics_queue) : device(device), swapchain(swapchain) {

    createRenderPass(swapchain->imageFormat);
    createFrameBuffers();
    createDescriptorPool(descriptor_allocator);

    deletion_queue.pushFunction([&]() {
        vkDestroyRenderPass(this->device, render_pass, nullptr);
        vkDestroyDescriptorPool(this->device, descriptor_pool, nullptr);
    });

    initImGui(physical_device, window, instance, grafics_queue_family, grafics_queue);

    gui_windows.push_back(std::make_shared<OptionsWindow>());
}

void GuiManager::createRenderPass(VkFormat image_format) {
    RenderPassBuilder renderPassBuilder;
    renderPassBuilder.setColorAttachmentFormat(image_format);
    render_pass = renderPassBuilder.createRenderPass(device);
}

void GuiManager::createFrameBuffers() {
    frame_buffers.resize(swapchain->imageViews.size());
    for (size_t i = 0; i < frame_buffers.size(); i++) {
        std::array<VkImageView, 1> attachments {
            swapchain->imageViews[i],
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = render_pass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapchain->extent.width;
        framebufferInfo.height = swapchain->extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &frame_buffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void GuiManager::createDescriptorPool(DescriptorAllocator descriptor_allocator) {
    std::vector<VkDescriptorPoolSize> pool_sizes =
        {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
    };
    descriptor_pool = descriptor_allocator.createPool(device, pool_sizes, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
}


void GuiManager::initImGui(VkPhysicalDevice physical_device, GLFWwindow* window, VkInstance instance,
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
    initInfo.ImageCount = static_cast<uint32_t>(swapchain->images.size());
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initInfo.Allocator = nullptr;
    initInfo.CheckVkResultFn = checkVulkanResult;
    ImGui_ImplVulkan_Init(&initInfo);
}

void GuiManager::updateWindow(std::shared_ptr<Swapchain> swapchain) {
    for (auto framebuffer : frame_buffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    this->swapchain = swapchain;
    createFrameBuffers();
}

void GuiManager::recordGuiCommands(VkCommandBuffer commandBuffer, ImDrawData* gui_draw2_data, uint32_t imageIndex) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    for (auto& window : gui_windows) {
        window->createFrame();
    }

    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();

    std::array<VkClearValue, 1> clearValues = {};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};

    VkRenderPassBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info.renderPass = render_pass;
    info.framebuffer = frame_buffers[imageIndex];
    info.renderArea.extent = swapchain->extent;
    info.clearValueCount = static_cast<uint32_t>(clearValues.size());
    info.pClearValues = clearValues.data();
    vkCmdBeginRenderPass(commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);

    // Record dear imgui primitives into command buffer
    ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);

    vkCmdEndRenderPass(commandBuffer);
}

void GuiManager::destroy() {
    shutdownImGui();

    for (auto framebuffer : frame_buffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    deletion_queue.flush();
}

void GuiManager::shutdownImGui() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
