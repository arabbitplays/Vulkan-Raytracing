#include <VulkanEngine.hpp>
#include <set>
#include <cstdlib>
#include <filesystem>
#include <HierarchyWindow.hpp>

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation",
    "VK_LAYER_PROFILER_unified",
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

void CmdTraceRaysKHR(VkDevice device, VkCommandBuffer commandBuffer, const VkStridedDeviceAddressRegionKHR* pRaygenShaderBindingTable,
    const VkStridedDeviceAddressRegionKHR* pMissShaderBindingTable, const VkStridedDeviceAddressRegionKHR* pHitShaderBindingTable,
    const VkStridedDeviceAddressRegionKHR* pCallableShaderBindingTable, uint32_t width, uint32_t height, uint32_t depth) {
    auto func = (PFN_vkCmdTraceRaysKHR) vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR");
    if (func != nullptr) {
        return func(commandBuffer, pRaygenShaderBindingTable, pMissShaderBindingTable,
            pHitShaderBindingTable, pCallableShaderBindingTable, width, height, depth);
    }
}

const uint32_t WIDTH = 6144;
const uint32_t HEIGHT = 3320;

void VulkanEngine::run(const std::string& config_file, const std::string& resources_dir) {
    context = std::make_shared<VulkanContext>();
    context->base_options = std::make_shared<BaseOptions>();
    context->base_options->resources_dir = resources_dir;
    properties_manager = std::make_shared<PropertiesManager>(config_file);

    initWindow();
    initVulkan();
    initGui();

    mainLoop();

    cleanup();
}


void VulkanEngine::initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
}

void VulkanEngine::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<VulkanEngine*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

void VulkanEngine::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto app = reinterpret_cast<VulkanEngine*>(glfwGetWindowUserPointer(window));
    if (app->scene_manager->scene != nullptr && app->scene_manager->scene->camera != nullptr) {
        app->scene_manager->scene->camera->processGlfwKeyEvent(key, action);
    }
}

void VulkanEngine::mouseCallback(GLFWwindow* window, double xPos, double yPos) {
    auto app = reinterpret_cast<VulkanEngine*>(glfwGetWindowUserPointer(window));
    if (app->scene_manager->scene != nullptr && app->scene_manager->scene->camera != nullptr) {
        app->scene_manager->scene->camera->processGlfwMouseEvent(xPos, yPos);
    }
}

void VulkanEngine::initGui() {
    QueueFamilyIndices indices = VulkanUtil::findQueueFamilies(physicalDevice, surface);
    guiManager = std::make_shared<GuiManager>(device, physicalDevice, window, instance, *descriptorAllocator,
        swapchain, indices.graphicsFamily.value(),
        graphicsQueue);

    mainDeletionQueue.pushFunction([&]() {
        guiManager->destroy();
    });

    guiManager->addWindow(std::make_shared<OptionsWindow>(properties_manager));
    auto inspector_window = std::make_shared<InspectorWindow>(properties_manager);
    guiManager->addWindow(inspector_window);
    hierarchy_window = std::make_shared<HierarchyWindow>(properties_manager, inspector_window);
    guiManager->addWindow(hierarchy_window);
}

void VulkanEngine::initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();

    initProperties();
    properties_manager->addPropertySection(renderer_properties);

    createCommandManager();
    createRessourceBuilder();
    createDescriptorAllocator();

    createSwapchain();

    context->device = device;
    context->physicalDevice = physicalDevice;
    context->instance = instance;
    context->surface = surface;
    context->swapchain = swapchain;
    context->resource_builder = pRessourceBuilder;
    context->descriptor_allocator = descriptorAllocator;
    context->command_manager = pCommandManager;
    context->texture_repository = std::make_shared<TextureRepository>(context->resource_builder);
    context->mesh_repository = std::make_shared<MeshRepository>(context);

    mainDeletionQueue.pushFunction([&]()
    {
        context->texture_repository->destroy();
        context->mesh_repository->destroy();
    });

    createRenderingTargets();

    mainDeletionQueue.pushFunction([&]() {
        cleanupRenderingTargets();
    });

    scene_manager = std::make_shared<SceneManager>(context, MAX_FRAMES_IN_FLIGHT, raytracingProperties);
    mainDeletionQueue.pushFunction([&]() {
        scene_manager->clearRessources();
    });

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
    QueueFamilyIndices indices = VulkanUtil::findQueueFamilies(device, surface);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = Swapchain::querySwapChainSupport(device, surface);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return extensionsSupported && indices.isComplete() && swapChainAdequate
        && deviceFeatures.samplerAnisotropy && deviceFeatures.shaderInt64 && deviceFeatures.shaderFloat64
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

void VulkanEngine::createLogicalDevice() {
    QueueFamilyIndices indices = VulkanUtil::findQueueFamilies(physicalDevice, surface);

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

void VulkanEngine::createCommandManager() {
    pCommandManager = std::make_shared<CommandManager>(device, VulkanUtil::findQueueFamilies(physicalDevice, surface));
    commandManager = *pCommandManager;

    mainDeletionQueue.pushFunction([&]() {
        commandManager.destroyCommandManager();
    });
}

void VulkanEngine::createRessourceBuilder() {
    pRessourceBuilder = std::make_shared<RessourceBuilder>(physicalDevice, device, commandManager, context->base_options->resources_dir);
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

    descriptorAllocator = std::make_shared<DescriptorAllocator>();
    descriptorAllocator->init(device, 4, poolRatios);

    mainDeletionQueue.pushFunction([&]() {
        descriptorAllocator->destroyPools(device);
    });
}

bool VulkanEngine::hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void VulkanEngine::createSwapchain() {
    swapchain = std::make_shared<Swapchain>(device, physicalDevice, window, surface, ressourceBuilder);
    mainDeletionQueue.pushFunction([&]() {
        swapchain->destroy();
    });
}

void VulkanEngine::createRenderingTargets() {
    render_targets.resize(MAX_FRAMES_IN_FLIGHT);
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        render_targets[i] = ressourceBuilder.createImage(
            VkExtent3D{swapchain->extent.width, swapchain->extent.height, 1},
            VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT);

        ressourceBuilder.transitionImageLayout(render_targets[i].image, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_ACCESS_NONE, VK_ACCESS_NONE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    }

    std::vector<uint32_t> pixels(swapchain->extent.width * swapchain->extent.height * 4);
    std::srand(time(0));
    for (int i = 0; i < swapchain->extent.width * swapchain->extent.height * 4; i++) {
        pixels[i] = std::rand();
    }
    rng_tex = ressourceBuilder.createImage(pixels.data(),
            VkExtent3D{swapchain->extent.width, swapchain->extent.height, 1},
            VK_FORMAT_R32G32B32A32_UINT, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_STORAGE_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_GENERAL);
}

AllocatedImage VulkanEngine::getRenderTarget()
{
    return render_targets[0];
}


void VulkanEngine::loadScene()
{
    assert(context->base_options->curr_scene_name != "");
    vkDeviceWaitIdle(device);
    properties_manager->curr_sample_count = 0;
    std::string path = context->base_options->resources_dir + "/scenes/" + context->base_options->curr_scene_name;
    scene_manager->createScene(path);
    scene_manager->curr_scene_name = context->base_options->curr_scene_name;
    properties_manager->addPropertySection(scene_manager->scene->material->getProperties());
    hierarchy_window->setScene(scene_manager->scene);

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

        if (scene_manager->curr_scene_name != context->base_options->curr_scene_name) {
            loadScene();
        }
        scene_manager->updateScene(mainDrawContext, currentFrame, getRenderTarget(), rng_tex);
        properties_manager->emitting_instances_count = scene_manager->getEmittingInstancesCount(); // TODO move this together with the creation of the instance buffers
        drawFrame();
    }

    vkDeviceWaitIdle(device);
}

void VulkanEngine::drawFrame() {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    int imageIndex = aquireNextSwapchainImage();
    if (imageIndex < 0)
        return;

    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    std::vector<VkSemaphore> waitSemaphore = {imageAvailableSemaphores[currentFrame]};
    std::vector<VkSemaphore> signalSemaphore = {renderFinishedSemaphores[currentFrame]};
    submitCommandBuffer(waitSemaphore, signalSemaphore);
    presentSwapchainImage(signalSemaphore, imageIndex);

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

int VulkanEngine::aquireNextSwapchainImage()
{
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapchain->handle, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        refreshAfterResize();
        return -1;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        std::runtime_error("failed to acquire swap chain image!");
    }
    return imageIndex;
}

void VulkanEngine::submitCommandBuffer(std::vector<VkSemaphore> wait_semaphore, std::vector<VkSemaphore> signal_semaphore)
{
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(wait_semaphore.size());
    submitInfo.pWaitSemaphores = wait_semaphore.data();
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
    submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signal_semaphore.size());;
    submitInfo.pSignalSemaphores = signal_semaphore.data();

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }
}

void VulkanEngine::presentSwapchainImage(std::vector<VkSemaphore> wait_semaphore, uint32_t image_index)
{
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = static_cast<uint32_t>(wait_semaphore.size());
    presentInfo.pWaitSemaphores = wait_semaphore.data();
    VkSwapchainKHR swapChains[] = {swapchain->handle};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &image_index;

    VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        refreshAfterResize();
        return;
    } else if (result != VK_SUCCESS) {
        std::runtime_error("failed to present swap chain image!");
    }
}

void VulkanEngine::refreshAfterResize() {
    vkDeviceWaitIdle(device);

    properties_manager->curr_sample_count = 0;
    swapchain->recreate();
    cleanupRenderingTargets();
    createRenderingTargets();
    guiManager->updateWindows(swapchain);
    scene_manager->updateScene(mainDrawContext, currentFrame, getRenderTarget(), rng_tex);}

void VulkanEngine::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    recordBeginCommandBuffer(commandBuffer);
    recordRenderToImage(commandBuffer);
    recordCopyToSwapchain(commandBuffer, imageIndex);
    guiManager->recordGuiCommands(commandBuffer, imageIndex);
    recordEndCommandBuffer(commandBuffer);
}

void VulkanEngine::recordBeginCommandBuffer(VkCommandBuffer commandBuffer)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin record command buffer!");
    }
}

void VulkanEngine::recordRenderToImage(VkCommandBuffer commandBuffer)
{
    Pipeline pipeline = *scene_manager->getMaterial()->pipeline;

    const uint32_t handleSizeAligned = VulkanUtil::alignedSize(raytracingProperties.shaderGroupHandleSize, raytracingProperties.shaderGroupHandleAlignment);

    VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry{};
    raygenShaderSbtEntry.deviceAddress = pipeline.raygenShaderBindingTable.deviceAddress;
    raygenShaderSbtEntry.stride = handleSizeAligned;
    raygenShaderSbtEntry.size = handleSizeAligned;

    VkStridedDeviceAddressRegionKHR missShaderSbtEntry{};
    missShaderSbtEntry.deviceAddress = pipeline.missShaderBindingTable.deviceAddress;
    missShaderSbtEntry.stride = handleSizeAligned;
    missShaderSbtEntry.size = handleSizeAligned;

    VkStridedDeviceAddressRegionKHR closestHitShaderSbtEntry{};
    closestHitShaderSbtEntry.deviceAddress = pipeline.hitShaderBindingTable.deviceAddress;
    closestHitShaderSbtEntry.stride = handleSizeAligned;
    closestHitShaderSbtEntry.size = handleSizeAligned;

    VkStridedDeviceAddressRegionKHR callableShaderSbtEntry{};

    std::vector<VkDescriptorSet> descriptor_sets{};
    descriptor_sets.push_back(scene_manager->scene_descriptor_sets[0]);
    descriptor_sets.push_back(scene_manager->getMaterial()->materialDescriptorSet);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline.getHandle());
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline.getLayoutHandle(),
        0, static_cast<uint32_t>(descriptor_sets.size()), descriptor_sets.data(),
        0, nullptr);

    uint32_t pc_size;
    void* pc_data = properties_manager->getPushConstants(&pc_size);
    vkCmdPushConstants(commandBuffer, pipeline.getLayoutHandle(), VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_KHR, 0, pc_size, pc_data);

    CmdTraceRaysKHR(
        device,
        commandBuffer,
        &raygenShaderSbtEntry,
        &missShaderSbtEntry,
        &closestHitShaderSbtEntry,
        &callableShaderSbtEntry,
        swapchain->extent.width,
        swapchain->extent.height,
        1);

    properties_manager->curr_sample_count++;
}

void VulkanEngine::recordCopyToSwapchain(VkCommandBuffer commandBuffer, uint32_t swapchain_image_index)
{
    AllocatedImage render_target = getRenderTarget();

    ressourceBuilder.transitionImageLayout(commandBuffer, swapchain->images[swapchain_image_index],
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_ACCESS_NONE, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    ressourceBuilder.transitionImageLayout(commandBuffer, render_target.image,
        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    int width = swapchain->extent.width;
    int height = swapchain->extent.height;

    VkImageBlit blitRegion{};
    blitRegion.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    blitRegion.srcOffsets[0] = {0, 0, 0};
    blitRegion.srcOffsets[1] = {width, height, 1};
    blitRegion.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    blitRegion.dstOffsets[0] = {0, 0, 0};
    blitRegion.dstOffsets[1] = {width, height, 1};

    vkCmdBlitImage(commandBuffer, render_target.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        swapchain->images[swapchain_image_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion, VK_FILTER_NEAREST);

    ressourceBuilder.transitionImageLayout(commandBuffer, swapchain->images[swapchain_image_index],
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_NONE,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    ressourceBuilder.transitionImageLayout(commandBuffer, render_target.image,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_NONE,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
}

void VulkanEngine::recordEndCommandBuffer(VkCommandBuffer commandBuffer)
{
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void VulkanEngine::cleanup() {
    mainDeletionQueue.flush();
    glfwDestroyWindow(window);
    glfwTerminate();
}

void VulkanEngine::cleanupRenderingTargets() {
    for (auto image : render_targets) {
        ressourceBuilder.destroyImage(image);
    }
    ressourceBuilder.destroyImage(rng_tex);
}

void VulkanEngine::outputRenderingTarget(const std::string& output_path)
{
    AllocatedImage render_target = getRenderTarget();
    void* data = context->resource_builder->downloadImage(render_target, sizeof(uint32_t));
    uint8_t* fixed_data = fixImageFormatForStorage(data, render_target.imageExtent.width * render_target.imageExtent.height, render_target.imageFormat);
    context->resource_builder->writePNG(output_path, fixed_data, render_target.imageExtent.width, render_target.imageExtent.height);

    delete fixed_data;
}

// target format is R8G8B8A8_UNORM
uint8_t* VulkanEngine::fixImageFormatForStorage(void* data, size_t pixel_count, VkFormat originalFormat)
{
    if (originalFormat == VK_FORMAT_R8G8B8A8_UNORM)
        return static_cast<uint8_t*>(data);

    if (originalFormat == VK_FORMAT_B8G8R8A8_UNORM)
    {
        auto image_data = static_cast<uint8_t*>(data);
        for (size_t i = 0; i < pixel_count; i++) {
            std::swap(image_data[i * 4], image_data[i * 4 + 2]);  // Swap B (0) and R (2)
        }
        return image_data;
    } if (originalFormat == VK_FORMAT_R32G32B32A32_SFLOAT)
    {
        uint8_t* output_image = new uint8_t[pixel_count * 4];
        auto image_data = static_cast<float*>(data);
        for (size_t i = 0; i < pixel_count * 4; i++) {
            float test = image_data[i];
            // Clamp each channel to the [0, 1] range and then scale to [0, 255]
            output_image[i] = static_cast<uint8_t>(std::fmin(1.0f, std::fmax(0.0f, image_data[i])) * 255);
        }
        delete image_data;
        return output_image;
    } else
    {
        spdlog::error("Image format of the storage image is not supported to be stored correctly!");
    }
}

void VulkanEngine::initProperties()
{
    renderer_properties = std::make_shared<Properties>(RENDERER_SECTION_NAME);

    renderer_properties->addString(RESOURCES_DIR_OPTION_NAME, &context->base_options->resources_dir);
    renderer_properties->addInt(RECURSION_DEPTH_OPTION_NAME, &context->base_options->max_depth, 1, 5);

    std::string scenes_dir = context->base_options->resources_dir + "/scenes";
    std::vector<std::string> scenes;
    try {
        for (const auto& entry : std::filesystem::__cxx11::directory_iterator(scenes_dir)) {
            scenes.push_back(entry.path().filename());
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("failed to load scene directory: " + std::string(e.what()));
    }

    if (scenes.empty())
    {
        throw std::runtime_error("No scenes found in scene directory " + scenes_dir + ".");
    }
    context->base_options->curr_scene_name = scenes[0];

    renderer_properties->addSelection(CURR_SCENE_OPTION_NAME, &context->base_options->curr_scene_name, scenes);
}
