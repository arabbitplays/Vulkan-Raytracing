//
// Created by oschdi on 4/27/25.
//

#ifndef DEVICEMANAGER_HPP
#define DEVICEMANAGER_HPP

#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <DeletionQueue.hpp>
#include <GLFW/glfw3.h>
#include <VulkanUtil.hpp>
#include <iostream>
#include <vector>

namespace RtEngine {
	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	enum QueueType {
		GRAPHICS,
		PRESENT,
	};

	class DeviceManager {
	public:
		static VkPhysicalDeviceRayTracingPipelinePropertiesKHR RAYTRACING_PROPERTIES;

		DeviceManager(GLFWwindow *window, bool enable_validation_layers);
		void destroy();

		[[nodiscard]] VkPhysicalDevice getPhysicalDevice() const;
		[[nodiscard]] VkDevice getDevice() const;
		[[nodiscard]] VkSurfaceKHR getSurface() const;
		[[nodiscard]] VkInstance getInstance() const;
		[[nodiscard]] QueueFamilyIndices getQueueIndices() const;
		[[nodiscard]] VkQueue getQueue(QueueType type) const;

	private:
		void createInstance(bool enable_validation_layers);
		bool checkValidationLayerSupport();
		std::vector<const char *> getRequiredExtensions(bool enable_validation_layers);

		void setupDebugMessenger();
		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);

		void createSurface(GLFWwindow *window);

		void pickPhysicalDevice();
		bool isDeviceSuitable(VkPhysicalDevice device);
		bool checkDeviceExtensionSupport(VkPhysicalDevice device);

		void createLogicalDevice(bool enable_validation_layers);

		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback([[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
															[[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
															const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
															[[maybe_unused]] void *pUserData) {

			std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

			return VK_FALSE;
		}

		DeletionQueue deletion_queue;

		VkInstance instance;
		VkSurfaceKHR surface;
		VkDevice device;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT debugMessenger;
		QueueFamilyIndices queue_indices;
		VkQueue graphics_queue, present_queue;
	};
} // namespace RtEngine

#endif // DEVICEMANAGER_HPP
