#define STB_IMAGE_IMPLEMENTATION
#include "rendering/engine/VulkanEngine.hpp"

#include <RaytracingPipelineBuilder.hpp>

#include "../nodes/MeshNode.hpp"
#include "../../Analytics.hpp"

#include "shader.vert.spv.h"
#include "shader.frag.spv.h"
#include "miss.rmiss.spv.h"
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

VkResult CreateRayTracingPipelinesKHR(VkDevice device, VkDeferredOperationKHR deferredOperation, VkPipelineCache pipelineCache, uint32_t createInfoCount,
    const VkRayTracingPipelineCreateInfoKHR* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines) {

    auto func = (PFN_vkCreateRayTracingPipelinesKHR) vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR");
    if (func != nullptr) {
        return func(device, deferredOperation, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
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
    createAccelerationStructureBuilder();

    createSwapChain();
    /*createDescriptorSetLayout();
    initPipelines();

    createDepthResources();
    initDefaultResources();*/

    loadMeshes();
    createUniformBuffers();
    createStorageImage();

    rt_createPipeline();
    createShaderBindingTables();
    rt_createDescriptorSets();

    //createDescriptorSets();

    //createFrameBuffers();
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

    // TODO check for raytracing features
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
    createDepthResources();
    createFrameBuffers();
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
    mainDeletionQueue.pushFunction([&]() {
        cleanupSwapChain();
    });
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

void VulkanEngine::createDescriptorSetLayout() {
    DescriptorLayoutBuilder layoutBuilder;

    layoutBuilder.clear();
    layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    sceneDataDescriptorLayout = layoutBuilder.build(device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

    mainDeletionQueue.pushFunction([&]() {
        vkDestroyDescriptorSetLayout(device, sceneDataDescriptorLayout, nullptr);
    });

    VkPushConstantRange range{};
    range.size = sizeof(ObjectData);
    range.offset = 0;
    range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    objectDataCostantRange = range;
}

void VulkanEngine::initPipelines() {
    metalRoughMaterial.buildPipelines(this);

    mainDeletionQueue.pushFunction([&]() {
        metalRoughMaterial.clearRessources(device);
    });
}

void VulkanEngine::createFrameBuffers() {
    swapChainFrameBuffers.resize(swapChainImageViews.size());
    for (size_t i = 0; i < swapChainFrameBuffers.size(); i++) {
        std::array<VkImageView, 2> attachments {
            swapChainImageViews[i],
            depthImage.imageView
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = defaultMetalRough.pipeline->renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFrameBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

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

void VulkanEngine::createAccelerationStructureBuilder() {
    auto pAsBuilder = new AccelerationStructureBuilder(device, ressourceBuilder, commandManager, mainDeletionQueue);
    acceleration_structure_builder = *pAsBuilder;
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
    defaultMetalRough = createMetalRoughMaterial(1.0f, 0.5f, glm::vec3{1, 1, 1});
}

MaterialInstance VulkanEngine::createMetalRoughMaterial(float metallic, float roughness, glm::vec3 albedo) {
    MetallicRoughness::MaterialResources resources{};
    resources.colorImage = whiteImage;
    resources.colorSampler = defaultSamplerLinear;
    resources.metalRoughImage = whiteImage;
    resources.metalRoughSampler = defaultSamplerLinear;

    MetallicRoughness::MaterialConstants constants{};
    constants.colorFactors = {albedo,1};
    constants.metalRoughFactors = {metallic, roughness, 0, 0};

    VkDeviceSize bufferSize = sizeof(MetallicRoughness::MaterialConstants);
    AllocatedBuffer materialBuffer = ressourceBuilder.createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    mainDeletionQueue.pushFunction([=, this]() {
        ressourceBuilder.destroyBuffer(materialBuffer);
    });

    void* data;
    vkMapMemory(device, materialBuffer.bufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, &constants, bufferSize);
    vkUnmapMemory(device, materialBuffer.bufferMemory);

    resources.dataBuffer = materialBuffer.handle;
    resources.bufferOffset = 0;

    return metalRoughMaterial.writeMaterial(device, MaterialPass::MainColor, resources, descriptorAllocator);
}

void VulkanEngine::createStorageImage() {
    storageImage = ressourceBuilder.createImage(
        VkExtent3D{swapChainExtent.width, swapChainExtent.height, 1},
        VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT);
    mainDeletionQueue.pushFunction([&]() {
        ressourceBuilder.destroyImage(storageImage);
    });

    ressourceBuilder.transitionImageLayout(storageImage.image, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_ACCESS_NONE, VK_ACCESS_NONE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
}

void VulkanEngine::loadMeshes() {
    auto pMeshAssetBuilder = new MeshAssetBuilder(device, ressourceBuilder);
    meshAssetBuilder = *pMeshAssetBuilder;

    MeshAsset meshAsset = meshAssetBuilder.LoadMeshAsset("Sphere", "../ressources/models/sphere.obj");
    meshAssets.push_back(meshAsset);

    for (auto& meshAsset : meshAssets) {
        mainDeletionQueue.pushFunction([&]() {
            meshAssetBuilder.destroyMeshAsset(meshAsset);
        });
    }

    createAccelerationStructures(meshAsset);

    /*std::shared_ptr<Node> parentNode = std::make_shared<Node>();
    parentNode->localTransform = glm::mat4{1.0f};
    parentNode->worldTransform = glm::mat4{1.0f};
    parentNode->children = {};
    loadedNodes["Spheres"] = std::move(parentNode);

    for (int x = 0; x < 5; x++) {
        for (int y = 0; y < 5; y++) {
            std::shared_ptr<MeshNode> newNode = std::make_shared<MeshNode>();
            newNode->meshAsset = std::make_shared<MeshAsset>(meshAssets[0]);

            MaterialInstance material = createMetalRoughMaterial(0.1f + 0.2f * (float)y, 0.01f + 0.2f * (float)x, glm::vec3{1, 1, 1});
            newNode->material = std::make_shared<Material>(material);

            glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3{x - 2, y - 2, 0});
            glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.4f));
            newNode->localTransform = translation * scale;
            newNode->worldTransform = glm::mat4{1.0f};

            loadedNodes["Spheres"]->children.push_back(newNode);
        }

        loadedNodes["Spheres"]->refreshTransform(glm::mat4(1.0f));
    }*/
}

void VulkanEngine::createAccelerationStructures(MeshAsset mesh) {
    bottomLevelAccelerationStructure = acceleration_structure_builder.buildBlasFromMesh(mesh);
    std::vector<AccelerationStructure> blas{bottomLevelAccelerationStructure};
    topLevelAccelerationStructure = acceleration_structure_builder.buildTlasFromMesh(blas);
    mainDeletionQueue.pushFunction([&]() {
        acceleration_structure_builder.destroyAccelerationStructure(bottomLevelAccelerationStructure);
        acceleration_structure_builder.destroyAccelerationStructure(topLevelAccelerationStructure);
    });
}

inline uint32_t alignedSize(uint32_t value, uint32_t alignment) {
    return (value + (alignment - 1)) & ~(alignment - 1);
}

void VulkanEngine::createShaderBindingTables() {
    const uint32_t handleSize = raytracingProperties.shaderGroupHandleSize;
    const uint32_t handleAlignment = raytracingProperties.shaderGroupHandleAlignment;
    const uint32_t handleSizeAligned = alignedSize(handleSize, handleAlignment);
    //const uint32_t groupCount = static_cast<uint32_t>(shaderGroups.size()); TODO Fix this
    const uint32_t groupCount = static_cast<uint32_t>(3);
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
    if (GetRayTracingShaderGroupHandlesKHR(device, rt_pipeline, 0, groupCount, sbt_size, shaderHandleStorage.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to get shader group handles!");
    }

    raygenShaderBindingTable.update(device, shaderHandleStorage.data(), handleSize);
    missShaderBindingTable.update(device, shaderHandleStorage.data() + handleSizeAligned, handleSize);
    hitShaderBindingTable.update(device, shaderHandleStorage.data() + 2* handleSizeAligned, handleSize);
}

void VulkanEngine::rt_createDescriptorSets() {
    rt_descriptorSet = descriptorAllocator.allocate(device, rt_descriptorSetLayout);
    descriptorAllocator.writeAccelerationStructure(0, topLevelAccelerationStructure.handle, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
    descriptorAllocator.writeImage(1, storageImage.imageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    descriptorAllocator.writeBuffer(2, sceneUniformBuffers[0].handle, sizeof(SceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    descriptorAllocator.updateSet(device, rt_descriptorSet);
}

void VulkanEngine::rt_createPipeline() {
    DescriptorLayoutBuilder layoutBuilder;
    RaytracingPipelineBuilder raytracingPipelineBuilder;

    layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
    layoutBuilder.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    layoutBuilder.addBinding(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    rt_descriptorSetLayout = layoutBuilder.build(device, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
    mainDeletionQueue.pushFunction([&]() {
        vkDestroyDescriptorSetLayout(device, rt_descriptorSetLayout, nullptr);
    });

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &rt_descriptorSetLayout;
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &rt_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
    mainDeletionQueue.pushFunction([&]() {
        vkDestroyPipelineLayout(device, rt_pipelineLayout, nullptr);
    });

    VkShaderModule raygenShaderModule = VulkanUtil::createShaderModule(device, oschd_raygen_rgen_spv_size(), oschd_raygen_rgen_spv());
    VkShaderModule missShaderModule = VulkanUtil::createShaderModule(device, oschd_miss_rmiss_spv_size(), oschd_miss_rmiss_spv());
    VkShaderModule closestHitShaderModule = VulkanUtil::createShaderModule(device, oschd_closesthit_rchit_spv_size(), oschd_closesthit_rchit_spv());

    raytracingPipelineBuilder.addShaderStage(raygenShaderModule, VK_SHADER_STAGE_RAYGEN_BIT_KHR, VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR);
    raytracingPipelineBuilder.addShaderStage(missShaderModule, VK_SHADER_STAGE_MISS_BIT_KHR, VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR);
    raytracingPipelineBuilder.addShaderStage(closestHitShaderModule, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR);

    VkRayTracingPipelineCreateInfoKHR pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    pipelineInfo.stageCount = static_cast<uint32_t>(raytracingPipelineBuilder.shader_stages.size());
    pipelineInfo.pStages = raytracingPipelineBuilder.shader_stages.data();
    pipelineInfo.groupCount = static_cast<uint32_t>(raytracingPipelineBuilder.shaderGroups.size());
    pipelineInfo.pGroups = raytracingPipelineBuilder.shaderGroups.data();
    pipelineInfo.maxPipelineRayRecursionDepth = 1;
    pipelineInfo.layout = rt_pipelineLayout;
    if (CreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &rt_pipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create ray tracing pipeline!");
    }
    mainDeletionQueue.pushFunction([&]() {
        vkDestroyPipeline(device, rt_pipeline, nullptr);
    });

    vkDestroyShaderModule(device, raygenShaderModule, nullptr);
    vkDestroyShaderModule(device, missShaderModule, nullptr);
    vkDestroyShaderModule(device, closestHitShaderModule, nullptr);
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

void VulkanEngine::createDescriptorAllocator() {
    std::vector<DescriptorAllocator::PoolSizeRatio> poolRatios = {
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
            { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1 },
    };
    descriptorAllocator.init(device, 4, poolRatios);

    mainDeletionQueue.pushFunction([&]() {
        descriptorAllocator.destroyPools(device);
    });
}

void VulkanEngine::createDescriptorSets() {
    sceneDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        sceneDescriptorSets[i] = descriptorAllocator.allocate(device, sceneDataDescriptorLayout, nullptr);
    }

    for (size_t i = 0; i < sceneDescriptorSets.size(); i++) {
        descriptorAllocator.clearWrites();
        descriptorAllocator.writeBuffer(0, sceneUniformBuffers[i].handle, sizeof(SceneData),
                                        0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        descriptorAllocator.updateSet(device, sceneDescriptorSets[i]);
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
    rt_recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

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
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    glm::mat4 rotation = glm::rotate(glm::mat4{1.0f}, time * glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    /*loadedNodes["Spheres"]->refreshTransform(rotation);

    mainDrawContext.opaqueSurfaces.clear();
    for (auto& pair : loadedNodes) {
        pair.second->draw(glm::mat4(1.0f), mainDrawContext);
    }*/

    SceneData sceneData{};
    sceneData.view = glm::translate(glm::mat4(1.0f), glm::vec3{ 0,0,-2.5f });
    /*sceneData.view = glm::lookAt(glm::vec3(4.0f, 4.0f, 4.0f),
                           glm::vec3(0.0f, 0.0f, 0.0f),
                           glm::vec3(0.0f, 0.0f, 1.0f));*/
    sceneData.proj = glm::perspective(glm::radians(60.0f),
                                swapChainExtent.width / (float) swapChainExtent.height,
                                0.1f, 512.0f);
    sceneData.proj[1][1] *= -1; // flip y-axis because glm is for openGL
    sceneData.viewProj = sceneData.view * sceneData.proj;

    sceneData.viewPos = glm::vec4{ 0,0,6, 0 };
    sceneData.pointLightPositions = {glm::vec4{1, 1, 3, 1}, glm::vec4{1, -1, 3, 1},
                                     glm::vec4{-1, 1, 3, 1}, glm::vec4{-1, -1, 3, 1}};
    sceneData.pointLightColors = {glm::vec4{1, 0, 0, 0}, glm::vec4{0, 1, 0, 0},
                                  glm::vec4{0, 0, 1, 0}, glm::vec4{1, 1, 1, 0}};

    sceneData.ambientColor = glm::vec4(1.f);
    sceneData.sunlightColor = glm::vec4(1.f);
    sceneData.sunlightDirection = glm::vec4(-1,1,2,1.f);

    memcpy(sceneUniformBuffersMapped[currentImage], &sceneData, sizeof(SceneData));
}

void VulkanEngine::rt_recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
// TODO fix storage image if it has not the right dims

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

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rt_pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rt_pipelineLayout, 0, 1, &rt_descriptorSet, 0, nullptr);

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

void VulkanEngine::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin record command buffer!");
    }

    MaterialPipeline pipeline = *defaultMetalRough.pipeline;

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = pipeline.renderPass;
    renderPassInfo.framebuffer = swapChainFrameBuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;

    std::array<VkClearValue, 2> clearValues = {};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline.pipelineLayout, 0, 1,
                            &sceneDescriptorSets[currentFrame], 0, nullptr);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChainExtent.width);
    viewport.height = static_cast<float>(swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    for (auto& renderObject : mainDrawContext.opaqueSurfaces) {
        VkBuffer vertexBuffers[] = {renderObject.vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

        vkCmdBindIndexBuffer(commandBuffer, renderObject.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipeline.pipelineLayout, 1, 1,
                                &renderObject.material->materialSet, 0, nullptr);

        ObjectData objectData{};
        objectData.model = renderObject.transform;
        vkCmdPushConstants(commandBuffer, pipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                           0, sizeof(ObjectData), &objectData);

        vkCmdDrawIndexed(commandBuffer, renderObject.indexCount, 1, renderObject.firstIndex, 0, 0);
    }

    vkCmdEndRenderPass(commandBuffer);
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
    for (auto framebuffer : swapChainFrameBuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    vkDestroySwapchainKHR(device, swapChain, nullptr);
}

VkFormat VulkanEngine::getColorAttachmentFormat() {
    return swapChainImageFormat;
}

VkFormat VulkanEngine::getDepthFormat() {
    return findDepthFormat();
}

/*void compileShaders(VkDevice* device, const std::string source_name, shaderc_shader_kind kind,
                    const std::string source, bool optimize) {

    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    if(optimize){
        options.SetOptimizationLevel(shaderc_optimization_level_size);

    }

    shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(source,kind,source_name.c_str(),options);
    if(module.GetCompilationStatus() != shaderc_compilation_status_success){
        LOG_E("Error compiling module - " << module.GetErrorMessage());
    }

    std::vector<uint32_t> spirv = {module.cbegin(),module.cend()};

    // build vulkan module
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spirv.size() * sizeof(unsigned int);
    createInfo.pCode = spirv.data();

    switch(kind){
        case shaderc_vertex_shader:

            if(vkCreateShaderModule(*device,&createInfo,nullptr,&vertexModule) != VK_SUCCESS){
                LOG_E("Error creating vertex module");
            }
            break;

        case shaderc_fragment_shader:
            if(vkCreateShaderModule(*device,&createInfo,nullptr,&vertexModule) != VK_SUCCESS){
                LOG_E("Error creating vertex module");
            }
            break;
 }
}*/

void MetallicRoughness::buildPipelines(VulkanEngine* engine) {
    VkShaderModule vertShader = VulkanUtil::createShaderModule(engine->device, oschd_shader_vert_spv_size(), oschd_shader_vert_spv());
    VkShaderModule fragShader = VulkanUtil::createShaderModule(engine->device, oschd_shader_frag_spv_size(), oschd_shader_frag_spv());

    DescriptorLayoutBuilder layoutBuilder;
    layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    layoutBuilder.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    layoutBuilder.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    materialLayout = layoutBuilder.build(engine->device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    std::vector<VkDescriptorSetLayout> layouts = {engine->sceneDataDescriptorLayout, materialLayout};

    std::vector<VkPushConstantRange> ranges{engine->objectDataCostantRange};

    RenderPassBuilder renderPassBuilder;
    renderPassBuilder.setColorAttachmentFormat(engine->getColorAttachmentFormat());
    renderPassBuilder.setDepthFormat(engine->getDepthFormat());
    opaquePipeline.renderPass = renderPassBuilder.createRenderPass(engine->device);
    transparentPipeline.renderPass = renderPassBuilder.createRenderPass(engine->device);

    RasterizerPipelineBuilder pipelineBuilder;
    pipelineBuilder.setDescriptorSetLayouts(layouts);
    pipelineBuilder.setPushConstantRanges(ranges);
    pipelineBuilder.setShaders(vertShader, fragShader);
    pipelineBuilder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
    pipelineBuilder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipelineBuilder.setMultisamplingNone();
    pipelineBuilder.disableColorBlending();
    pipelineBuilder.enableDepthTest(VK_TRUE, VK_COMPARE_OP_LESS);

    pipelineBuilder.buildPipelineLayout(engine->device, &opaquePipeline.pipelineLayout);
    pipelineBuilder.buildPipeline(engine->device, opaquePipeline.renderPass, &opaquePipeline.pipeline, opaquePipeline.pipelineLayout);

    pipelineBuilder.enableAdditiveBlending();
    pipelineBuilder.enableDepthTest(VK_FALSE, VK_COMPARE_OP_LESS);

    pipelineBuilder.buildPipelineLayout(engine->device, &transparentPipeline.pipelineLayout);
    pipelineBuilder.buildPipeline(engine->device, transparentPipeline.renderPass, &transparentPipeline.pipeline, transparentPipeline.pipelineLayout);

    vkDestroyShaderModule(engine->device, vertShader, nullptr);
    vkDestroyShaderModule(engine->device, fragShader, nullptr);
}

MaterialInstance MetallicRoughness::writeMaterial(VkDevice device, MaterialPass pass,
                                                  const MaterialResources& resources, DescriptorAllocator& allocator) {
    MaterialInstance instance;
    instance.materialPass = pass;
    if (pass == MaterialPass::Transparent) {
        instance.pipeline = &transparentPipeline;
    } else {
        instance.pipeline = &opaquePipeline;
    }

    instance.materialSet = allocator.allocate(device, materialLayout);

    allocator.clearWrites();
    allocator.writeBuffer(0, resources.dataBuffer, sizeof(MaterialConstants), resources.bufferOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    allocator.writeImage(1, resources.colorImage.imageView, resources.colorSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    allocator.writeImage(2, resources.metalRoughImage.imageView, resources.metalRoughSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    allocator.updateSet(device, instance.materialSet);

    return instance;
}

void MetallicRoughness::clearRessources(VkDevice device) {
    vkDestroyDescriptorSetLayout(device, materialLayout, nullptr);
    vkDestroyPipeline(device, opaquePipeline.pipeline, nullptr);
    vkDestroyPipeline(device, transparentPipeline.pipeline, nullptr);
    vkDestroyPipelineLayout(device, opaquePipeline.pipelineLayout, nullptr);
    vkDestroyPipelineLayout(device, transparentPipeline.pipelineLayout, nullptr);
    vkDestroyRenderPass(device, opaquePipeline.renderPass, nullptr);
    vkDestroyRenderPass(device, transparentPipeline.renderPass, nullptr);
}
