#ifndef BASICS_VULKANENGINE_HPP
#define BASICS_VULKANENGINE_HPP

#include <iostream>

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
#include <BaseOptions.hpp>
#include <HierarchyWindow.hpp>

#include "../scene_graph/Node.hpp"
#include "DeletionQueue.hpp"
#include <VulkanContext.hpp>


namespace RtEngine {

class VulkanEngine {
public:
    VulkanEngine() = default;
    virtual ~VulkanEngine() = default;
    void run(const std::string& config_file, const std::string& resources_dir);
protected:
    GLFWwindow* window;
    std::shared_ptr<GuiManager> guiManager;

    DeletionQueue mainDeletionQueue;

    std::shared_ptr<VulkanContext> context;

    std::vector<VkCommandBuffer> commandBuffers;

    DrawContext mainDrawContext;
    std::shared_ptr<PropertiesManager> properties_manager;
    std::shared_ptr<Properties> renderer_properties;

    std::vector<AllocatedImage> render_targets;
    std::vector<AllocatedImage> rng_textures;

    std::shared_ptr<SceneManager> scene_manager;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    uint32_t currentFrame = 0;
    bool framebufferResized = false;

    uint32_t max_frames_in_flight = 1;

    void initWindow();
    void initVulkan();
    void createContext();
    void initGui();
    virtual void mainLoop();
    void cleanup();

    static void framebufferResizeCallback(GLFWwindow *window, int width, int height);
    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
    static void mouseCallback(GLFWwindow *window, double xPos, double yPos);


    void createImageViews();

    void createGuiFrameBuffers();

    bool hasStencilComponent(VkFormat format);

    std::shared_ptr<DescriptorAllocator> createDescriptorAllocator();
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
    virtual AllocatedImage getRngTexture();
    void cleanupRenderingTargets();

    void outputRenderingTarget(const std::string& output_path);
    uint8_t* fixImageFormatForStorage(void* image_data, size_t pixel_count, VkFormat originalFormat);

    virtual void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void recordBeginCommandBuffer(VkCommandBuffer commandBuffer);
    void recordRenderToImage(VkCommandBuffer commandBuffer);
    void recordCopyToSwapchain(VkCommandBuffer commandBuffer, uint32_t swapchain_image_index);
    void recordEndCommandBuffer(VkCommandBuffer commandBuffer);

    void loadScene();
    virtual void initProperties();
    void initSceneSelectionProperty();
};

}
#endif //BASICS_VULKANENGINE_HPP

