#include "GuiManager.hpp"

#include <RenderPassBuilder.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <stdexcept>

// only used as imgui callback
namespace RtEngine {
	void checkVulkanResult(VkResult err) {
		if (err == 0)
			return;
		throw std::runtime_error("Error: VkResult = " + err);
	}

	GuiManager::GuiManager(std::shared_ptr<VulkanContext> context) : context(context) {
		VkDevice device = context->device_manager->getDevice();
		createRenderPass(device, context->swapchain->imageFormat);
		createFrameBuffers(device, context->swapchain);
		createDescriptorPool(device, context->descriptor_allocator);

		deletion_queue.pushFunction([&]() {
			vkDestroyRenderPass(this->context->device_manager->getDevice(), render_pass, nullptr);
			vkDestroyDescriptorPool(this->context->device_manager->getDevice(), descriptor_pool, nullptr);
		});

		initImGui(context->device_manager, context->window, context->swapchain);
	}

	void GuiManager::createRenderPass(VkDevice device, VkFormat image_format) {
		RenderPassBuilder renderPassBuilder;
		renderPassBuilder.setColorAttachmentFormat(image_format);
		render_pass = renderPassBuilder.createRenderPass(device);
	}

	void GuiManager::createFrameBuffers(VkDevice device, std::shared_ptr<Swapchain> swapchain) {
		frame_buffers.resize(swapchain->imageViews.size());
		for (size_t i = 0; i < frame_buffers.size(); i++) {
			std::array<VkImageView, 1> attachments{
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

	void GuiManager::createDescriptorPool(VkDevice device, std::shared_ptr<DescriptorAllocator> descriptor_allocator) {
		std::vector<VkDescriptorPoolSize> pool_sizes = {
				{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
		};
		descriptor_pool =
				descriptor_allocator->createPool(device, pool_sizes, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
	}

	void GuiManager::initImGui(std::shared_ptr<DeviceManager> device_manager, GLFWwindow *window,
							   std::shared_ptr<Swapchain> swapchain) {
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO &io = ImGui::GetIO();
		(void) io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

		ImGui::StyleColorsDark();
		// ImGui::StyleColorsLight();

		ImGui_ImplGlfw_InitForVulkan(window, true);
		ImGui_ImplVulkan_InitInfo initInfo = {};
		initInfo.Instance = device_manager->getInstance();
		initInfo.PhysicalDevice = device_manager->getPhysicalDevice();
		initInfo.Device = device_manager->getDevice();
		initInfo.QueueFamily = device_manager->getQueueIndices().graphicsFamily.value();
		initInfo.Queue = device_manager->getQueue(GRAPHICS);
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

	void GuiManager::addWindow(std::shared_ptr<GuiWindow> window) { gui_windows.push_back(window); }

	void GuiManager::updateWindows() {
		for (auto framebuffer: frame_buffers) {
			vkDestroyFramebuffer(context->device_manager->getDevice(), framebuffer, nullptr);
		}

		createFrameBuffers(context->device_manager->getDevice(), context->swapchain);
	}

	void GuiManager::recordGuiCommands(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		for (auto &window: gui_windows) {
			window->createFrame();
		}

		ImGui::Render();
		ImDrawData *draw_data = ImGui::GetDrawData();

		std::array<VkClearValue, 1> clearValues = {};
		clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};

		VkRenderPassBeginInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		info.renderPass = render_pass;
		info.framebuffer = frame_buffers[imageIndex];
		info.renderArea.extent = context->swapchain->extent;
		info.clearValueCount = static_cast<uint32_t>(clearValues.size());
		info.pClearValues = clearValues.data();
		vkCmdBeginRenderPass(commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);

		// Record dear imgui primitives into command buffer
		ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);

		vkCmdEndRenderPass(commandBuffer);
	}

	void GuiManager::destroy() {
		shutdownImGui();

		for (auto framebuffer: frame_buffers) {
			vkDestroyFramebuffer(context->device_manager->getDevice(), framebuffer, nullptr);
		}
		deletion_queue.flush();
	}

	void GuiManager::shutdownImGui() {
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
} // namespace RtEngine
