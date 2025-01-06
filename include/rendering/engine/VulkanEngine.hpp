//
// Created by oster on 09.09.2024.
//

#ifndef BASICS_VULKANENGINE_HPP
#define BASICS_VULKANENGINE_HPP

#include <iostream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <optional>
#include <fstream>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>
#include <chrono>
#include <stb_image.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include <AccelerationStructure.hpp>
#include <GuiManager.hpp>
#include <GuiWindow.hpp>
#include <imgui_impl_vulkan.h>
#include <PhongMaterial.hpp>
#include <Scene.hpp>
#include <SceneManager.hpp>
#include <Swapchain.hpp>
#include <unordered_map>
#include "../Vertex.hpp"
#include "DescriptorAllocator.hpp"
#include "CommandManager.hpp"
#include "../../builders/MeshAssetBuilder.hpp"
#include "../../util/QuickTimer.hpp"
#include "../../util/VulkanUtil.hpp"
#include "../IRenderable.hpp"
#include "../nodes/Node.hpp"
#include "DeletionQueue.hpp"
#include <VulkanContext.hpp>

class VulkanEngine {
public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    VkDevice device;
    std::shared_ptr<CommandManager> pCommandManager;
    CommandManager commandManager;
    std::shared_ptr<RessourceBuilder> pRessourceBuilder;
    RessourceBuilder ressourceBuilder;
    std::shared_ptr<MeshAssetBuilder> mesh_builder;
    std::shared_ptr<DescriptorAllocator> descriptorAllocator;

    void run();
    VkFormat getColorAttachmentFormat();
    VkFormat getDepthFormat();

private:

    GLFWwindow* window;
    std::shared_ptr<GuiManager> guiManager;
    VkInstance instance;
    VkSurfaceKHR surface;
    VkDebugUtilsMessengerEXT debugMessenger;
    DeletionQueue mainDeletionQueue;

    std::shared_ptr<VulkanContext> context;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkQueue graphicsQueue, presentQueue;

    std::shared_ptr<Swapchain> swapchain;

    AllocatedImage depthImage;

    std::vector<VkCommandBuffer> commandBuffers;

    DrawContext mainDrawContext;
    std::shared_ptr<RaytracingOptions> raytracing_options;
    std::shared_ptr<RendererOptions> renderer_options;

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR raytracingProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};

    std::vector<AllocatedImage> storageImages;

    std::shared_ptr<SceneManager> scene_manager;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    uint32_t currentFrame = 0;
    bool framebufferResized = false;

    std::shared_ptr<MaterialInstance> default_phong;

    void initWindow();
    void initVulkan();
    void initGui();
    void mainLoop();
    void cleanup();

    static void framebufferResizeCallback(GLFWwindow *window, int width, int height);
    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
    static void mouseCallback(GLFWwindow *window, double xPos, double yPos);

    void createInstance();
    bool checkValidationLayerSupport();
    std::vector<const char *> getRequiredExtensions();
    void setupDebugMessenger();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
    void createSurface();
    void pickPhysicalDevice();
    bool isDeviceSuitable(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    void createLogicalDevice();
    void createImageViews();

    void createGuiFrameBuffers();

    void createCommandManager();
    void createRessourceBuilder();

    void createDepthResources();
    VkFormat findDepthFormat();
    VkFormat findSupportedFormat(const std::vector<VkFormat> candidates, VkImageTiling tiling,
                                 VkFormatFeatureFlags features);
    bool hasStencilComponent(VkFormat format);
    AllocatedImage loadTextureImage(std::string path);


    void createSwapchain();

    void createDescriptorAllocator();
    void createCommandBuffers();
    void createSyncObjects();

    void pollSdlEvents();

    void drawFrame();

    void refreshAfterResize();

    void cleanupStorageImages();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData) {

        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }

    void createStorageImages();
};

#endif //BASICS_VULKANENGINE_HPP

