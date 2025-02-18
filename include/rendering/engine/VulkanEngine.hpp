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
#include "../scene_graph/Node.hpp"
#include "DeletionQueue.hpp"
#include <VulkanContext.hpp>

class VulkanEngine {
public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 1;

    VkDevice device;
    std::shared_ptr<CommandManager> pCommandManager;
    CommandManager commandManager;
    std::shared_ptr<RessourceBuilder> pRessourceBuilder;
    RessourceBuilder ressourceBuilder;
    std::shared_ptr<MeshAssetBuilder> mesh_builder;
    std::shared_ptr<DescriptorAllocator> descriptorAllocator;

    void run(RendererOptions& options);
protected:

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

    std::vector<VkCommandBuffer> commandBuffers;

    DrawContext mainDrawContext;
    std::shared_ptr<RaytracingOptions> raytracing_options;
    std::shared_ptr<RendererOptions> renderer_options;

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR raytracingProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};

    std::vector<AllocatedImage> render_targets;
    AllocatedImage rng_tex;

    std::shared_ptr<SceneManager> scene_manager;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    uint32_t currentFrame = 0;
    bool framebufferResized = false;

    void initWindow();
    void initVulkan();
    void initGui();
    virtual void mainLoop();
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

    bool hasStencilComponent(VkFormat format);


    void createSwapchain();

    void createDescriptorAllocator();
    void createCommandBuffers();
    void createSyncObjects();

    void pollSdlEvents();

    virtual void drawFrame();
    int aquireNextSwapchainImage();
    void submitCommandBuffer(std::vector<VkSemaphore> wait_semaphore, std::vector<VkSemaphore> signal_semaphore);
    void presentSwapchainImage(std::vector<VkSemaphore> wait_semaphore, uint32_t image_index);

    void refreshAfterResize();

    void createRenderingTargets();
    virtual AllocatedImage getRenderTarget();
    void outputRenderingTarget();
    void fixImageFormatForStorage(unsigned char* image_data, size_t pixel_count, VkFormat originalFormat);
    void cleanupRenderingTargets();

    virtual void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void recordBeginCommandBuffer(VkCommandBuffer commandBuffer);
    void recordRenderToImage(VkCommandBuffer commandBuffer);
    void recordCopyToSwapchain(VkCommandBuffer commandBuffer, uint32_t swapchain_image_index);
    void recordEndCommandBuffer(VkCommandBuffer commandBuffer);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData) {

        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }

    virtual void loadScene();
};

#endif //BASICS_VULKANENGINE_HPP

