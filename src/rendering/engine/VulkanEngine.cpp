#define STB_IMAGE_IMPLEMENTATION
#include "rendering/engine/VulkanEngine.hpp"

#include <DescriptorLayoutBuilder.hpp>
#include <PhongMaterial.hpp>
#include <set>

#include "../nodes/MeshNode.hpp"
#include "../../Analytics.hpp"

#include "miss.rmiss.spv.h"
#include "shadow_miss.rmiss.spv.h"
#include "closesthit.rchit.spv.h"
#include "raygen.rgen.spv.h"

const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
};

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                      const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}



VkResult GetRayTracingShaderGroupHandlesKHR(VkDevice device, VkPipeline pipeline, uint32_t firstGroup, uint32_t groupCount, size_t dataSize, void* pData) {
    auto func = (PFN_vkGetRayTracingShaderGroupHandlesKHR) vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR");
    if (func != nullptr) {
        return func(device, pipeline, firstGroup, groupCount, dataSize, pData);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void CmdTraceRaysKHR(VkDevice device, VkCommandBuffer commandBuffer, const VkStridedDeviceAddressRegionKHR* pRaygenShaderBindingTable,
    const VkStridedDeviceAddressRegionKHR* pMissShaderBindingTable, const VkStridedDeviceAddressRegionKHR* pHitShaderBindingTable,
    const VkStridedDeviceAddressRegionKHR* pCallableShaderBindingTable, uint32_t width, uint32_t height, uint32_t depth) {
    auto func = (PFN_vkCmdTraceRaysKHR) vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR");
    if (func != nullptr) {
        return func(commandBuffer, pRaygenShaderBindingTable, pMissShaderBindingTable,
            pHitShaderBindingTable, pCallableShaderBindingTable, width, height, depth);
    }
}

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 2;

void VulkanEngine::run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}


void VulkanEngine::initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void VulkanEngine::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<VulkanEngine*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

void VulkanEngine::initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();

    createCommandManager();
    createRessourceBuilder();
    createDescriptorAllocator();

    createSwapChain();
    createStorageImage();

    mainDeletionQueue.pushFunction([&]() {
        cleanupSwapChain();
    });

    createSceneLayout();

    //createDepthResources();
    initDefaultResources();

    loadMeshes();
    createSceneBuffers();
    createAccelerationStructure();
    createUniformBuffers();

    createShaderBindingTables();
    rt_createDescriptorSets();

    createCommandBuffers();
    createSyncObjects();
}

void VulkanEngine::createInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
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
    std::cout << "available extensions:\n";
    for (const auto& extension : extensions) {
        std::cout << "\t" << extension.extensionName << "\n";
    }

    std::vector<const char*> glfwExtensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(glfwExtensions.size());
    createInfo.ppEnabledExtensionNames = glfwExtensions.data();

    // Add an extra debug messenger for the create and destroy instance calls
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }

    mainDeletionQueue.pushFunction([&]() {
        vkDestroyInstance(instance, nullptr);
    });
}

bool VulkanEngine::checkValidationLayerSupport() {
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for(const char* layerName : validationLayers) {
        bool layerFound = false;
        for (const auto& layerProperties : availableLayers) {
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

std::vector<const char*> VulkanEngine::getRequiredExtensions() {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

void VulkanEngine::setupDebugMessenger() {
    if (!enableValidationLayers)
        return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }

    mainDeletionQueue.pushFunction([&]() {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    });

    auto pAnalytics = new Analytics();
    analytics = *pAnalytics;


}

void VulkanEngine::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo. sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
#ifdef VERBOSE
    createInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
#endif
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr; // Optional data that is passed via the pUserData parameter to the callback
}

void VulkanEngine::createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }

    mainDeletionQueue.pushFunction([&]() {
        vkDestroySurfaceKHR(instance, surface, nullptr);
    });
}

void VulkanEngine::pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0)
        throw std::runtime_error("failed to find GPUs with Vulkan support!");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    for (auto& device : devices) {
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
    physicalDeviceProperties.pNext = &raytracingProperties;
    vkGetPhysicalDeviceProperties2(physicalDevice, &physicalDeviceProperties);
}

bool VulkanEngine::isDeviceSuitable(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR raytracingPipelineFeatures {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
    raytracingPipelineFeatures.pNext = &accelerationStructureFeatures;
    VkPhysicalDeviceFeatures2 deviceFeatures2;
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.pNext = &raytracingPipelineFeatures;
    vkGetPhysicalDeviceFeatures2(device, &deviceFeatures2);

    // implement device checks here

    bool extensionsSupported = checkDeviceExtensionSupport(device);
    QueueFamilyIndices indices = findQueueFamilies(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return extensionsSupported && indices.isComplete() && swapChainAdequate
        && deviceFeatures.samplerAnisotropy
        && raytracingPipelineFeatures.rayTracingPipeline && accelerationStructureFeatures.accelerationStructure;
}

bool VulkanEngine::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

QueueFamilyIndices VulkanEngine::findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for(const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }

        i++;
    }
    return indices;
}

VulkanEngine::SwapChainSupportDetails VulkanEngine::querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

void VulkanEngine::createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    // so no family is created multiple times if it covers multiple types
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

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
    if (enableValidationLayers) { // device validation alyers are deprecated, only set for compatibility
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }
    createInfo.pNext = &deviceAddressFeatures;

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    mainDeletionQueue.pushFunction([&]() {
        vkDestroyDevice(device, nullptr);
    });

    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

void VulkanEngine::recreateSwapChain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);
    cleanupSwapChain();

    createSwapChain();
    //createDepthResources();
    createStorageImage();
    descriptorAllocator.writeImage(1, storageImage.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    descriptorAllocator.updateSet(device, scene_descriptor_set);
    descriptorAllocator.clearWrites();
}

void VulkanEngine::createSwapChain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtend(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0
            && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;

    createImageViews();
}

VkSurfaceFormatKHR VulkanEngine::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB
                && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR VulkanEngine::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanEngine::chooseSwapExtend(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;
    else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width,
                                        capabilities.minImageExtent.width,
                                        capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height,
                                        capabilities.minImageExtent.height,
                                        capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void VulkanEngine::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        swapChainImageViews[i] = ressourceBuilder.createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

/*void VulkanEngine::createDescriptorSetLayout() {

    VkPushConstantRange range{};
    range.size = sizeof(ObjectData);
    range.offset = 0;
    range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    objectDataCostantRange = range;
}*/

void VulkanEngine::createCommandManager() {
    auto pCommandManager = new CommandManager(device, findQueueFamilies(physicalDevice));
    commandManager = *pCommandManager;

    mainDeletionQueue.pushFunction([&]() {
        commandManager.destroyCommandManager();
    });
}

void VulkanEngine::createRessourceBuilder() {
    auto pRessourceBuilder = new RessourceBuilder(physicalDevice, device, commandManager);
    ressourceBuilder = *pRessourceBuilder;
}

void VulkanEngine::createDescriptorAllocator() {
    std::vector<DescriptorAllocator::PoolSizeRatio> poolRatios = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
        { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1 },
};
    descriptorAllocator.init(device, 4, poolRatios);

    mainDeletionQueue.pushFunction([&]() {
        descriptorAllocator.destroyPools(device);
    });
}

void VulkanEngine::createDepthResources() {
    VkFormat depthFormat = findDepthFormat();

    depthImage = ressourceBuilder.createImage({swapChainExtent.width, swapChainExtent.height, 1}, depthFormat,
                                              VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);

    mainDeletionQueue.pushFunction([&]() {
        ressourceBuilder.destroyImage(depthImage);
    });
}

VkFormat VulkanEngine::findDepthFormat() {
    return findSupportedFormat(
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkFormat VulkanEngine::findSupportedFormat(const std::vector<VkFormat> candidates, VkImageTiling tiling,
                             VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &properties);
        if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("no supported format found!");
}

bool VulkanEngine::hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

AllocatedImage VulkanEngine::loadTextureImage(std::string path) {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    AllocatedImage textureImage = ressourceBuilder.createImage(pixels, {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1},
                                                VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                                                VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

    stbi_image_free(pixels);

    mainDeletionQueue.pushFunction([=, this]() {
        ressourceBuilder.destroyImage(textureImage);
    });

    return textureImage;
}

void VulkanEngine::initDefaultResources() {
    createDefaultTextures();
    createDefaultSamplers();
    createDefaultMaterials();
}

void VulkanEngine::createDefaultTextures() {
    uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
    whiteImage = ressourceBuilder.createImage((void*)&white, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_SRGB,
                               VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

    uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
    greyImage = ressourceBuilder.createImage((void*)&grey, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_SRGB,
                                              VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

    uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
    blackImage = ressourceBuilder.createImage((void*)&black, VkExtent3D{ 1, 1, 1 }, VK_FORMAT_R8G8B8A8_SRGB,
                                               VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

    //checkerboard image
    uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
    std::array<uint32_t, 16 *16 > pixels; //for 16x16 checkerboard texture
    for (int x = 0; x < 16; x++) {
        for (int y = 0; y < 16; y++) {
            pixels[y*16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
        }
    }
    errorCheckerboardImage = ressourceBuilder.createImage(pixels.data(), VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_SRGB,
                                           VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

    mainDeletionQueue.pushFunction([&]() {
        ressourceBuilder.destroyImage(whiteImage);
        ressourceBuilder.destroyImage(greyImage);
        ressourceBuilder.destroyImage(blackImage);
        ressourceBuilder.destroyImage(errorCheckerboardImage);
    });
}

void VulkanEngine::createDefaultSamplers() {
    VkSamplerCreateInfo samplerInfo = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &defaultSamplerNearest) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &defaultSamplerLinear) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }

    mainDeletionQueue.pushFunction([&]() {
        vkDestroySampler(device, defaultSamplerLinear, nullptr);
        vkDestroySampler(device, defaultSamplerNearest, nullptr);
    });
}

void VulkanEngine::createDefaultMaterials() {
    phong_material = std::make_shared<PhongMaterial>(device, ressourceBuilder);
    phong_material->buildPipelines(rt_descriptorSetLayout);
    mainDeletionQueue.pushFunction([&]() {
        phong_material->clearRessources();
    });
    //default_phong = createPhongMaterial(glm::vec3{1, 0, 0}, 1, 1, 1);
}

std::shared_ptr<MaterialInstance> VulkanEngine::createPhongMaterial(glm::vec3 albedo, float diffuse, float specular, float ambient) {
    auto constants = std::make_shared<PhongMaterial::MaterialConstants>();
    constants->albedo = {albedo,1};
    constants->properies = {diffuse, specular, ambient, 0};

    auto ressources = std::make_shared<PhongMaterial::MaterialRessources>();
    ressources->constants = constants;
    return phong_material->addInstance(ressources);
    /*uint32_t size = sizeof(PhongMaterial::MaterialConstants);
    PhongMaterial::MaterialRessources ressources {};
    ressources.data_buffer = ressourceBuilder.createBuffer(size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    ressources.data_buffer.update(device, &constants, size);

    return phong_material->writeMaterial(ressources);*/
    return nullptr;
}

void VulkanEngine::createStorageImage() {
    storageImage = ressourceBuilder.createImage(
        VkExtent3D{swapChainExtent.width, swapChainExtent.height, 1},
        VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT);

    ressourceBuilder.transitionImageLayout(storageImage.image, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_ACCESS_NONE, VK_ACCESS_NONE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
}

void VulkanEngine::loadMeshes() {
    auto pMeshAssetBuilder = new MeshAssetBuilder(device, ressourceBuilder);
    meshAssetBuilder = *pMeshAssetBuilder;

    std::shared_ptr<MeshAsset> meshAsset = std::make_shared<MeshAsset>(meshAssetBuilder.LoadMeshAsset("Sphere", "../ressources/models/sphere.obj"));
    meshAssets.push_back(meshAsset);

    meshAsset = std::make_shared<MeshAsset>(meshAssetBuilder.LoadMeshAsset("Sphere", "../ressources/models/plane.obj"));
    meshAssets.push_back(meshAsset);

    for (auto& meshAsset : meshAssets) {
        mainDeletionQueue.pushFunction([&]() {
            meshAssetBuilder.destroyMeshAsset(*meshAsset);
        });
    }

    std::shared_ptr<MeshNode> sphere1 = std::make_shared<MeshNode>();
    sphere1->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(1.5f, 0.0f, 0.0f));
    sphere1->worldTransform = glm::mat4{1.0f};
    sphere1->children = {};
    sphere1->meshAsset = meshAssets[0];
    sphere1->meshMaterial = createPhongMaterial(glm::vec3(1.0f, 0.0f, 0.0f), 1.0f, 0.0f, 0.0f);
    sphere1->refreshTransform(glm::mat4(1.0f));
    loadedNodes["Sphere1"] = std::move(sphere1);

    /*std::shared_ptr<MeshNode> sphere2 = std::make_shared<MeshNode>();
    sphere2->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(-1.5f, 0.0f, 0.0f));
    sphere2->worldTransform = glm::mat4{1.0f};
    sphere2->children = {};
    sphere2->meshAsset = meshAssets[0];
    sphere2->meshMaterial = createPhongMaterial(glm::vec3(0.0f, 1.0f, 0.0f), 1.0f, 0.0f, 0.0f);
    sphere2->refreshTransform(glm::mat4(1.0f));
    loadedNodes["Sphere2"] = std::move(sphere2);*/

    std::shared_ptr<MeshNode> plane = std::make_shared<MeshNode>();
    plane->localTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f)) * glm::scale( glm::mat4(1.0f), glm::vec3(8.f, 1.0f, 8.f));
    plane->worldTransform = glm::mat4{1.0f};
    plane->children = {};
    plane->meshAsset = meshAssets[1];
    plane->meshMaterial = createPhongMaterial(glm::vec3(0.0f, 1.0f, 0.0f), 1.0f, 0.0f, 0.0f);
    plane->refreshTransform(glm::mat4(1.0f));
    loadedNodes["Plane"] = std::move(plane);

    /*std::shared_ptr<Node> parentNode = std::make_shared<Node>();
    parentNode->localTransform = glm::mat4{1.0f};
    parentNode->worldTransform = glm::mat4{1.0f};
    parentNode->children = {};
    loadedNodes["Spheres"] = std::move(parentNode);

    for (int x = 0; x < 5; x++) {
        for (int y = 0; y < 5; y++) {
            std::shared_ptr<MeshNode> newNode = std::make_shared<MeshNode>();
            newNode->meshAsset = meshAssets[0];

            //MaterialInstance material = createMetalRoughMaterial(0.1f + 0.2f * (float)y, 0.01f + 0.2f * (float)x, glm::vec3{1, 1, 1});
            //newNode->material = std::make_shared<Material>(material);

            glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3{x - 2, y - 2, 0});
            glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.4f));
            newNode->localTransform = translation * scale;
            newNode->worldTransform = glm::mat4{1.0f};

            loadedNodes["Spheres"]->children.push_back(newNode);
        }

        loadedNodes["Spheres"]->refreshTransform(glm::mat4(1.0f));
    }*/
}

void VulkanEngine::createSceneBuffers() {
    vertex_buffer = meshAssetBuilder.createVertexBuffer(meshAssets);
    index_buffer = meshAssetBuilder.createIndexBuffer(meshAssets);
    phong_material->writeMaterial();

    geometry_mapping_buffer = meshAssetBuilder.createGeometryMappingBuffer(meshAssets);

    mainDeletionQueue.pushFunction([&]() {
        ressourceBuilder.destroyBuffer(vertex_buffer);
        ressourceBuilder.destroyBuffer(index_buffer);
        ressourceBuilder.destroyBuffer(geometry_mapping_buffer);
    });
}

void VulkanEngine::createAccelerationStructure() {
    QuickTimer timer{"BLAS Build", true};
    uint32_t object_id = 0;
    for (auto& meshAsset : meshAssets) {
        meshAsset->accelerationStructure = std::make_shared<AccelerationStructure>(device, ressourceBuilder, commandManager, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR);

        meshAsset->accelerationStructure->addTriangleGeometry(vertex_buffer, index_buffer,
            meshAsset->vertex_count - 1, meshAsset->triangle_count, sizeof(Vertex),
            meshAsset->instance_data.vertex_offset, meshAsset->instance_data.triangle_offset);
        meshAsset->accelerationStructure->build();
        meshAsset->geometry_id = object_id++;
    }
}

inline uint32_t alignedSize(uint32_t value, uint32_t alignment) {
    return (value + (alignment - 1)) & ~(alignment - 1);
}

void VulkanEngine::createShaderBindingTables() {
    Pipeline pipeline = *phong_material->pipeline;

    std::vector<uint32_t> rgen_indices{0};
    std::vector<uint32_t> miss_indices{1, 2};
    std::vector<uint32_t> hit_indices{3};

    const uint32_t handleSize = raytracingProperties.shaderGroupHandleSize;
    const uint32_t handleAlignment = raytracingProperties.shaderGroupHandleAlignment;
    const uint32_t handleSizeAligned = alignedSize(handleSize, handleAlignment);
    const uint32_t groupCount = static_cast<uint32_t>(pipeline.getGroupCount());
    const uint32_t sbt_size = groupCount * handleSizeAligned;
    const uint32_t sbtUsageFlags = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    const uint32_t sbtMemoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    raygenShaderBindingTable = ressourceBuilder.createBuffer(handleSize, sbtUsageFlags, sbtMemoryPropertyFlags);
    missShaderBindingTable = ressourceBuilder.createBuffer(handleSize, sbtUsageFlags, sbtMemoryPropertyFlags);
    hitShaderBindingTable = ressourceBuilder.createBuffer(handleSize, sbtUsageFlags, sbtMemoryPropertyFlags);
    mainDeletionQueue.pushFunction([&]() {
        ressourceBuilder.destroyBuffer(raygenShaderBindingTable);
        ressourceBuilder.destroyBuffer(missShaderBindingTable);
        ressourceBuilder.destroyBuffer(hitShaderBindingTable);
    });

    std::vector<uint8_t> shaderHandleStorage(sbt_size);
    if (GetRayTracingShaderGroupHandlesKHR(device, pipeline.getHandle(), 0, groupCount, sbt_size, shaderHandleStorage.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to get shader group handles!");
    }

    auto copyHandle = [&](AllocatedBuffer& buffer, std::vector<uint32_t>& indices, uint32_t stride) {
        size_t offset = 0;
        for (uint32_t index = 0; index < indices.size(); index++) {
            buffer.update(device, shaderHandleStorage.data() + (indices[index] * handleSizeAligned), handleSize, offset);
            offset += stride * sizeof(uint8_t);
        }
    };

    copyHandle(raygenShaderBindingTable, rgen_indices, handleSizeAligned);
    copyHandle(missShaderBindingTable, miss_indices, handleSizeAligned);
    copyHandle(hitShaderBindingTable, hit_indices, handleSizeAligned);
}

void VulkanEngine::rt_createDescriptorSets() {
    // TODO make this MAX_FRAMES_IN_FLIGHT many
    scene_descriptor_set = descriptorAllocator.allocate(device, rt_descriptorSetLayout);
    // Binding 0 is the TLAS added in updateScene
    descriptorAllocator.writeImage(1, storageImage.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    descriptorAllocator.writeBuffer(2, sceneUniformBuffers[0].handle, sizeof(SceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    descriptorAllocator.writeBuffer(3, vertex_buffer.handle, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    descriptorAllocator.writeBuffer(4, index_buffer.handle, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    descriptorAllocator.writeBuffer(5, geometry_mapping_buffer.handle, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    descriptorAllocator.updateSet(device, scene_descriptor_set);
    descriptorAllocator.clearWrites();
}

void VulkanEngine::createSceneLayout() {
    DescriptorLayoutBuilder layoutBuilder;

    layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
    layoutBuilder.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    layoutBuilder.addBinding(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    layoutBuilder.addBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    layoutBuilder.addBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    layoutBuilder.addBinding(5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    layoutBuilder.addBinding(6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    layoutBuilder.addBinding(7, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);

    rt_descriptorSetLayout = layoutBuilder.build(device, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
    mainDeletionQueue.pushFunction([&]() {
        vkDestroyDescriptorSetLayout(device, rt_descriptorSetLayout, nullptr);
    });
}


void VulkanEngine::createUniformBuffers() {
    VkDeviceSize size = sizeof(SceneData);

    sceneUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    sceneUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        sceneUniformBuffers[i] = ressourceBuilder.createBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        vkMapMemory(device, sceneUniformBuffers[i].bufferMemory, 0, size, 0, &sceneUniformBuffersMapped[i]);

        mainDeletionQueue.pushFunction([&, i]() {
            ressourceBuilder.destroyBuffer(sceneUniformBuffers[i]);
        });
    }
}

void VulkanEngine::createCommandBuffers() {
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandManager.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command bufer!");
    }
}

void VulkanEngine::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS
            || vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS
            || vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create sync objects");
        }

        mainDeletionQueue.pushFunction([&, i]() {
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            vkDestroyFence(device, inFlightFences[i], nullptr);
        });
    }
}

void VulkanEngine::mainLoop() {
    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }

    vkDeviceWaitIdle(device);
}

void VulkanEngine::drawFrame() {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    //analytics.endFrame();
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
    //analytics.startFrame();

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        std::runtime_error("failed to acquire swap chain image!");
    }

    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    updateScene(currentFrame);

    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphore[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphore;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
    VkSemaphore signalSemaphore[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphore;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphore;
    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS) {
        std::runtime_error("failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanEngine::updateScene(uint32_t currentImage) {
    //QuickTimer timer{"Scene Update", true};

    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    glm::mat4 rotation = glm::rotate(glm::mat4{1.0f}, time * glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    if (top_level_acceleration_structure == nullptr) {
        top_level_acceleration_structure = std::make_shared<AccelerationStructure>(device, ressourceBuilder, commandManager, VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR);

        mainDeletionQueue.pushFunction([&]() {
            top_level_acceleration_structure->destroy();
        });
    }

    mainDrawContext.objects.clear();
    for (auto& pair : loadedNodes) {
        pair.second->draw(glm::mat4(1.0f), mainDrawContext);
    }

    if (instance_mapping_buffer.handle == VK_NULL_HANDLE) { // TODO make it dynamic
        instance_mapping_buffer = meshAssetBuilder.createInstanceMappingBuffer(mainDrawContext.objects);
        mainDeletionQueue.pushFunction([&]() {
            ressourceBuilder.destroyBuffer(instance_mapping_buffer);
        });
    }
    uint32_t instance_id = 0;
    for (auto& object : mainDrawContext.objects) {
        top_level_acceleration_structure->addInstance(object.acceleration_structure, object.transform, instance_id++);
    }

    glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(1.5f, 0.0f, 0.0f));
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f));

    translation = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, -0.5f, 0.0f));
    scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.8f, 0.8f, 0.8f));
    rotation = glm::rotate(glm::mat4{1.0f}, glm::radians(-90.f), glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 rotation2 = glm::rotate(glm::mat4{1.0f}, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    if (top_level_acceleration_structure->getHandle() == VK_NULL_HANDLE) {
        top_level_acceleration_structure->addInstanceGeometry();
    } else {
        top_level_acceleration_structure->update_instance_geometry(0);
    }
    top_level_acceleration_structure->build();

    descriptorAllocator.writeAccelerationStructure(0, top_level_acceleration_structure->getHandle(), VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
    descriptorAllocator.writeBuffer(6, instance_mapping_buffer.handle, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    descriptorAllocator.updateSet(device, scene_descriptor_set);
    descriptorAllocator.clearWrites();

    SceneData sceneData{};
    sceneData.view = glm::translate(glm::mat4(1.0f), glm::vec3{ 0,0,-4.5f });
    /*sceneData.view = glm::lookAt(glm::vec3(4.0f, 4.0f, 4.0f),
                           glm::vec3(0.0f, 0.0f, 0.0f),
                           glm::vec3(0.0f, 0.0f, 1.0f));*/
    sceneData.proj = glm::perspective(glm::radians(60.0f),
                                swapChainExtent.width / (float) swapChainExtent.height,
                                0.1f, 512.0f);
    sceneData.proj[1][1] *= -1; // flip y-axis because glm is for openGL
    sceneData.viewProj = sceneData.view * sceneData.proj;

    sceneData.viewPos = glm::vec4{ 0,0,6, 0 };
    sceneData.pointLightPositions = {glm::vec4{-2, 4, 3, 1}, glm::vec4{1, -1, 3, 1},
                                     glm::vec4{-1, 1, 3, 1}, glm::vec4{-1, -1, 3, 1}};
    sceneData.pointLightColors = {glm::vec4{1, 0, 0, 0}, glm::vec4{0, 1, 0, 0},
                                  glm::vec4{0, 0, 1, 0}, glm::vec4{1, 1, 1, 0}};

    sceneData.ambientColor = glm::vec4(1.f);
    sceneData.sunlightColor = glm::vec4(1.f);
    sceneData.sunlightDirection = glm::vec4(-1,-1,-1,1.f);

    memcpy(sceneUniformBuffersMapped[currentImage], &sceneData, sizeof(SceneData));
}

void VulkanEngine::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    Pipeline pipeline = *phong_material->pipeline;

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin record command buffer!");
    }

    const uint32_t handleSizeAligned = alignedSize(raytracingProperties.shaderGroupHandleSize, raytracingProperties.shaderGroupHandleAlignment);

    VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry{};
    raygenShaderSbtEntry.deviceAddress = raygenShaderBindingTable.deviceAddress;
    raygenShaderSbtEntry.stride = handleSizeAligned;
    raygenShaderSbtEntry.size = handleSizeAligned;

    VkStridedDeviceAddressRegionKHR missShaderSbtEntry{};
    missShaderSbtEntry.deviceAddress = missShaderBindingTable.deviceAddress;
    missShaderSbtEntry.stride = handleSizeAligned;
    missShaderSbtEntry.size = handleSizeAligned;

    VkStridedDeviceAddressRegionKHR closestHitShaderSbtEntry{};
    closestHitShaderSbtEntry.deviceAddress = hitShaderBindingTable.deviceAddress;
    closestHitShaderSbtEntry.stride = handleSizeAligned;
    closestHitShaderSbtEntry.size = handleSizeAligned;

    VkStridedDeviceAddressRegionKHR callableShaderSbtEntry{};

    std::vector<VkDescriptorSet> descriptor_sets{};
    descriptor_sets.push_back(scene_descriptor_set);
    descriptor_sets.push_back(phong_material->materialDescriptorSet);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline.getHandle());
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline.getLayoutHandle(),
        0, static_cast<uint32_t>(descriptor_sets.size()), descriptor_sets.data(),
        0, nullptr);

    CmdTraceRaysKHR(
        device,
        commandBuffer,
        &raygenShaderSbtEntry,
        &missShaderSbtEntry,
        &closestHitShaderSbtEntry,
        &callableShaderSbtEntry,
        swapChainExtent.width,
        swapChainExtent.height,
        1);

    ressourceBuilder.transitionImageLayout(commandBuffer, swapChainImages[imageIndex],
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_ACCESS_NONE, VK_ACCESS_NONE,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    ressourceBuilder.transitionImageLayout(commandBuffer, storageImage.image, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_ACCESS_NONE, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    VkImageCopy copyRegion{};
    copyRegion.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copyRegion.srcOffset = {0, 0, 0};
    copyRegion.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copyRegion.dstOffset = {0, 0, 0};
    copyRegion.extent = {swapChainExtent.width, swapChainExtent.height, 1};
    vkCmdCopyImage(commandBuffer, storageImage.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        swapChainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    ressourceBuilder.transitionImageLayout(commandBuffer, swapChainImages[imageIndex],
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_ACCESS_NONE, VK_ACCESS_NONE,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    ressourceBuilder.transitionImageLayout(commandBuffer, storageImage.image,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_NONE,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void VulkanEngine::cleanup() {
    mainDeletionQueue.flush();
    glfwDestroyWindow(window);
    glfwTerminate();
}

void VulkanEngine::cleanupSwapChain() {
    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    vkDestroySwapchainKHR(device, swapChain, nullptr);

    ressourceBuilder.destroyImage(storageImage);
}

VkFormat VulkanEngine::getColorAttachmentFormat() {
    return swapChainImageFormat;
}

VkFormat VulkanEngine::getDepthFormat() {
    return findDepthFormat();
}
