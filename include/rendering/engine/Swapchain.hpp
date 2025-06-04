#ifndef SWAPCHAIN_HPP
#define SWAPCHAIN_HPP

#include <DeviceManager.hpp>
#include <GLFW/glfw3.h>
#include <vector>
#include <vulkan/vulkan_core.h>
#include "ResourceBuilder.hpp"

namespace RtEngine {
	class Swapchain {
	public:
		Swapchain() = default;
		Swapchain(std::shared_ptr<DeviceManager> device_manager, GLFWwindow *window,
				  std::shared_ptr<ResourceBuilder> resource_builder) :
			device_manager(device_manager), resource_builder(resource_builder) {
			createSwapchain();
		};
		static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
		void recreate();
		void destroy();

		std::shared_ptr<DeviceManager> device_manager;
		GLFWwindow *window;
		std::shared_ptr<ResourceBuilder> resource_builder;

		VkSwapchainKHR handle;
		std::vector<VkImage> images;
		std::vector<VkImageView> imageViews;
		VkFormat imageFormat;
		VkExtent2D extent;

	private:
		void createSwapchain();
		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
		VkExtent2D chooseSwapExtend(const VkSurfaceCapabilitiesKHR &capabilities);
		void createImageViews();
	};

} // namespace RtEngine
#endif // SWAPCHAIN_HPP
