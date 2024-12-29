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
#include <Pipeline.hpp>
#include <Scene.hpp>
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
#include "../../Analytics.hpp"
#include "DeletionQueue.hpp"

class VulkanEngine {
public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    VkDevice device;
    CommandManager commandManager;
    RessourceBuilder ressourceBuilder;
    std::shared_ptr<MeshAssetBuilder> mesh_builder;

    VkDescriptorSetLayout sceneDataDescriptorLayout;

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

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkQueue graphicsQueue, presentQueue;

    std::shared_ptr<Swapchain> swapchain;

    AllocatedImage depthImage;

    std::vector<VkCommandBuffer> commandBuffers;

    AllocatedBuffer vertex_buffer, index_buffer, instance_mapping_buffer, geometry_mapping_buffer;
    std::shared_ptr<AccelerationStructure> top_level_acceleration_structure;

    DrawContext mainDrawContext;
    std::shared_ptr<RaytracingOptions> raytracing_options;

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR raytracingProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};

    std::vector<AllocatedImage> storageImages;

    std::shared_ptr<Scene> scene;

    AllocatedBuffer raygenShaderBindingTable;
    AllocatedBuffer missShaderBindingTable;
    AllocatedBuffer hitShaderBindingTable;

    AllocatedImage whiteImage;
    AllocatedImage greyImage;
    AllocatedImage blackImage;
    AllocatedImage errorCheckerboardImage;

    VkSampler defaultSamplerLinear;
    VkSampler defaultSamplerNearest;

    std::vector<VkDescriptorSet> scene_descriptor_sets{};
    VkDescriptorSetLayout scene_descsriptor_set_layout;
    std::vector<AllocatedBuffer> sceneUniformBuffers;
    std::vector<void*> sceneUniformBuffersMapped;

    DescriptorAllocator descriptorAllocator;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    uint32_t currentFrame = 0;
    bool framebufferResized = false;

    std::shared_ptr<MaterialInstance> default_phong;
    std::shared_ptr<PhongMaterial> phong_material;

    void initWindow();

    void createSceneBuffers();

    void initVulkan();
    void mainLoop();
    void cleanup();

    static void framebufferResizeCallback(GLFWwindow *window, int width, int height);
    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
    static void mouseCallback(GLFWwindow *window, double xPos, double yPos);

    void initGui();

    void createInstance();
    bool checkValidationLayerSupport();
    std::vector<const char *> getRequiredExtensions();
    void setupDebugMessenger();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
    void createSurface();
    void pickPhysicalDevice();
    bool isDeviceSuitable(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

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
    void initDefaultResources();
    void createDefaultTextures();
    void createDefaultSamplers();
    void createDefaultMaterials();

    void createSwapchain();

    void createScene();

    void createAccelerationStructure();
    void createShaderBindingTables();

    void createSceneDescriptorSets();

    void createSceneLayout();

    void createUniformBuffers();
    void createDescriptorAllocator();
    void createCommandBuffers();
    void createSyncObjects();

    void pollSdlEvents();

    void drawFrame();

    void refreshAfterResize();

    void cleanupStorageImages();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, ImDrawData* gui_draw_data);

    void updateScene(uint32_t currentImage);

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

