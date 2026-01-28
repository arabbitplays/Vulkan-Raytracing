//
// Created by oschdi on 4/27/25.
//

#include <DeviceManager.hpp>
#include <Swapchain.hpp>
#include <VulkanUtil.hpp>
#include <cstring>
#include <set>

namespace RtEngine {
	const std::vector<const char *> validationLayers = {
			"VK_LAYER_KHRONOS_validation",
		//	"VK_LAYER_PROFILER_unified",
	};

	const std::vector<const char *> deviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
			VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
			VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
			VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
	};

	VkPhysicalDeviceRayTracingPipelinePropertiesKHR DeviceManager::RAYTRACING_PROPERTIES{
			VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};

	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
										  const VkAllocationCallbacks *pAllocator,
										  VkDebugUtilsMessengerEXT *pDebugMessenger) {
		auto func =
				(PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		} else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
									   const VkAllocationCallbacks *pAllocator) {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance,
																				"vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(instance, debugMessenger, pAllocator);
		}
	}

	// --------------------------------------------------------------------------------------------------------------------

	DeviceManager::DeviceManager(GLFWwindow *window, bool enable_validation_layers) {
		createInstance(enable_validation_layers);
		if (enable_validation_layers) {
			setupDebugMessenger();
		}
		createSurface(window);
		pickPhysicalDevice();
		createLogicalDevice(enable_validation_layers);
	}

	VkPhysicalDevice DeviceManager::getPhysicalDevice() const { return physicalDevice; }

	VkDevice DeviceManager::getDevice() const { return device; }

	VkSurfaceKHR DeviceManager::getSurface() const { return surface; }

	VkInstance DeviceManager::getInstance() const { return instance; }

	QueueFamilyIndices DeviceManager::getQueueIndices() const { return queue_indices; }

	VkQueue DeviceManager::getQueue(QueueType type) const {
		switch (type) {
			case QueueType::GRAPHICS:
				return graphics_queue;
			case QueueType::PRESENT:
				return present_queue;
			case QueueType::COMPUTE:
				return compute_queue;
		}

		throw std::runtime_error("Unknown QueueType");
	}

	void DeviceManager::createInstance(bool enable_validation_layers) {
		if (enable_validation_layers && !checkValidationLayerSupport()) {
			throw std::runtime_error("validation layers requested, but not available!");
		}

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_3;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

		std::vector<const char *> glfwExtensions = getRequiredExtensions(enable_validation_layers);
		createInfo.enabledExtensionCount = static_cast<uint32_t>(glfwExtensions.size());
		createInfo.ppEnabledExtensionNames = glfwExtensions.data();

		// Add an extra debug messenger for the create and destroy instance calls
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (enable_validation_layers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugCreateInfo;
		} else {
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}

		deletion_queue.pushFunction([&]() { vkDestroyInstance(instance, nullptr); });
	}

	bool DeviceManager::checkValidationLayerSupport() {
		uint32_t layerCount = 0;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char *layerName: validationLayers) {
			bool layerFound = false;
			for (const auto &layerProperties: availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				return false;
			}
		}

		return true;
	}

	std::vector<const char *> DeviceManager::getRequiredExtensions(bool enable_validation_layers) {
		uint32_t glfwExtensionCount = 0;
		const char **glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (enable_validation_layers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	void DeviceManager::setupDebugMessenger() {
		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		populateDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug messenger!");
		}

		deletion_queue.pushFunction([&]() { DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr); });
	}

	void DeviceManager::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity =
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
#ifdef VERBOSE
		createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
#endif
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
								 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
								 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = nullptr; // Optional data that is passed via the pUserData parameter to the callback
	}

	void DeviceManager::createSurface(GLFWwindow *window) {
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}

		deletion_queue.pushFunction([&]() { vkDestroySurfaceKHR(instance, surface, nullptr); });
	}

	void DeviceManager::pickPhysicalDevice() {
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		if (deviceCount == 0)
			throw std::runtime_error("failed to find GPUs with Vulkan support!");

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
		for (auto &device: devices) {
			if (isDeviceSuitable(device)) {
				physicalDevice = device;
				break;
			}
		}

		if (physicalDevice == VK_NULL_HANDLE) {
			throw std::runtime_error("failed to find suitable GPU!");
		}

		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
		VkPhysicalDeviceProperties2 physicalDeviceProperties;
		physicalDeviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		physicalDeviceProperties.pNext = &RAYTRACING_PROPERTIES;
		vkGetPhysicalDeviceProperties2(physicalDevice, &physicalDeviceProperties);
	}

	bool DeviceManager::isDeviceSuitable(VkPhysicalDevice device) {
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		VkPhysicalDeviceRayTracingPipelineFeaturesKHR raytracingPipelineFeatures{
				VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
		VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{
				VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
		raytracingPipelineFeatures.pNext = &accelerationStructureFeatures;
		VkPhysicalDeviceFeatures2 deviceFeatures2;
		deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		deviceFeatures2.pNext = &raytracingPipelineFeatures;
		vkGetPhysicalDeviceFeatures2(device, &deviceFeatures2);

		// implement device checks here

		bool extensionsSupported = checkDeviceExtensionSupport(device);
		QueueFamilyIndices indices = VulkanUtil::findQueueFamilies(device, surface);

		bool swapChainAdequate = false;
		if (extensionsSupported) {
			SwapChainSupportDetails swapChainSupport = Swapchain::querySwapChainSupport(device, surface);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		return extensionsSupported && indices.isComplete() && swapChainAdequate && deviceFeatures.samplerAnisotropy &&
			   deviceFeatures.shaderInt64 && deviceFeatures.shaderFloat64 &&
			   raytracingPipelineFeatures.rayTracingPipeline && accelerationStructureFeatures.accelerationStructure;
	}

	bool DeviceManager::checkDeviceExtensionSupport(VkPhysicalDevice device) {
		uint32_t extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
		for (const auto &extension: availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	void DeviceManager::createLogicalDevice(bool enable_validation_layers) {
		queue_indices = VulkanUtil::findQueueFamilies(physicalDevice, surface);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		// so no family is created multiple times if it covers multiple types
		std::set<uint32_t> uniqueQueueFamilies = {queue_indices.graphicsAndComputeFamily.value(),
												  queue_indices.presentFamily.value()};

		float queuePriority = 1.0f;
		for (uint32_t queueFamily: uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;

			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;
		deviceFeatures.shaderInt64 = VK_TRUE;
		deviceFeatures.shaderFloat64 = VK_TRUE;

		VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
		accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
		accelerationStructureFeatures.accelerationStructure = VK_TRUE;

		VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{};
		rayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
		rayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
		rayTracingPipelineFeatures.pNext = &accelerationStructureFeatures;

		VkPhysicalDeviceBufferDeviceAddressFeatures deviceAddressFeatures{};
		deviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
		deviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
		deviceAddressFeatures.pNext = &rayTracingPipelineFeatures;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();
		if (enable_validation_layers) { // device validation alyers are deprecated, only set for compatibility
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		} else {
			createInfo.enabledLayerCount = 0;
		}
		createInfo.pNext = &deviceAddressFeatures;

		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device!");
		}

		deletion_queue.pushFunction([&]() { vkDestroyDevice(device, nullptr); });

		vkGetDeviceQueue(device, queue_indices.graphicsAndComputeFamily.value(), 0, &graphics_queue);
		vkGetDeviceQueue(device, queue_indices.presentFamily.value(), 0, &present_queue);
		vkGetDeviceQueue(device, queue_indices.graphicsAndComputeFamily.value(), 0, &compute_queue);
	}

	void DeviceManager::destroy() { deletion_queue.flush(); }

} // namespace RtEngine
