#ifndef GUIMANAGER_HPP
#define GUIMANAGER_HPP

#include <DescriptorAllocator.hpp>
#include <GLFW/glfw3.h>
#include <Swapchain.hpp>
#include <VulkanContext.hpp>
#include <bits/shared_ptr.h>
#include <imgui.h>
#include <vulkan/vulkan_core.h>
#include "DeletionQueue.hpp"

namespace RtEngine {
	class GuiRenderer {
	public:
		GuiRenderer() = default;
		GuiRenderer(std::shared_ptr<VulkanContext> context);

		void addWindow(std::shared_ptr<GuiWindow> window);
		void recreateFramebuffer();
		void recordGuiCommands(VkCommandBuffer commandBuffer, uint32_t imageIndex);
		void cleanup();

		VkRenderPass render_pass;
		std::vector<VkFramebuffer> frame_buffers;

	private:
		void createRenderPass(VkDevice device, VkFormat image_format);
		void createFrameBuffers(VkDevice device, std::shared_ptr<Swapchain> swapchain);
		void createDescriptorPool(VkDevice device);
		void initImGui(std::shared_ptr<DeviceManager> device_manager, GLFWwindow *window,
					   std::shared_ptr<Swapchain> swapchain);
		void shutdownImGui();

		std::shared_ptr<VulkanContext> context;

		std::vector<std::shared_ptr<GuiWindow>> gui_windows{};

		uint32_t minImageCount = 2;
		VkDescriptorPool descriptor_pool;

		DeletionQueue deletion_queue{};

		bool show_demo_window = true;
		bool show_main_window = true;
	};

} // namespace RtEngine
#endif // GUIWINDOW_HPP
