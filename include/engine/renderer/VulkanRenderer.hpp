#ifndef BASICS_VULKANENGINE_HPP
#define BASICS_VULKANENGINE_HPP

#include <vector>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <chrono>
#define TINYOBJLOADER_IMPLEMENTATION
#include <AccelerationStructure.hpp>
#include <GuiManager.hpp>
#include <GuiWindow.hpp>
#include <RenderTarget.hpp>
#include <../renderer/vulkan_scene_representation/SceneAdapter.hpp>
#include "../../util/QuickTimer.hpp"
#include "DescriptorAllocator.hpp"
#include "SceneAdapter.hpp"

#include <RuntimeContext.hpp>
#include <VulkanContext.hpp>
#include "../scene_graph/Node.hpp"
#include "DeletionQueue.hpp"
#include "Window.hpp"

namespace RtEngine {

	class VulkanRenderer {
	public:
		VulkanRenderer() = default;
		virtual ~VulkanRenderer() = default;
		void init(const std::shared_ptr<BaseOptions> &base_options, std::shared_ptr<Window> window);

		void loadScene(std::shared_ptr<IScene> scene);
		void update();

		void waitForNextFrameStart();
		void resetCurrFrame();

		int32_t aquireNextSwapchainImage();
		void recordCommands(bool present, int32_t swapchain_image_idx);
		void submitCommands(bool present, int32_t swapchain_image_idx);

		void cleanup();

		std::shared_ptr<RuntimeContext> getRuntimeContext();
		std::unordered_map<std::string, std::shared_ptr<Material>> getMaterials() const;

		std::shared_ptr<RenderTarget> getRenderTarget() const;

		std::shared_ptr<PropertiesManager> getPropertiesManager();

		void handleGuiUpdate(uint32_t update_flags) const;

		std::shared_ptr<VulkanContext> getVulkanContext();

		VkExtent2D getSwapchainExtent();

	protected:
		std::shared_ptr<Window> window;

		DeletionQueue mainDeletionQueue;

		std::shared_ptr<BaseOptions> base_options;

		std::shared_ptr<VulkanContext> vulkan_context;
		std::shared_ptr<RuntimeContext> runtime_context;

		std::vector<VkCommandBuffer> commandBuffers;

		std::shared_ptr<DrawContext> mainDrawContext;
		std::shared_ptr<PropertiesManager> properties_manager;
		std::shared_ptr<PropertiesSection> renderer_properties;

		std::shared_ptr<SceneAdapter> scene_manager;

		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		std::vector<VkFence> inFlightFences;

		bool framebufferResized = false;

		uint32_t max_frames_in_flight = 1;

		void initWindow();
		void initVulkan();
		void createVulkanContext();
		void createRuntimeContext();

		void createMainDrawContext();

		std::shared_ptr<RenderTarget> createRenderTarget();

		static bool hasStencilComponent(VkFormat format);

		std::shared_ptr<DescriptorAllocator> createDescriptorAllocator();
		void createCommandBuffers();
		void createSyncObjects();

		void pollSdlEvents();

		void submitCommandBuffer(std::vector<VkSemaphore> wait_semaphore, std::vector<VkSemaphore> signal_semaphore);
		void presentSwapchainImage(const std::vector<VkSemaphore>& wait_semaphore, uint32_t image_index);

		void refreshAfterResize();

		void outputRenderingTarget(const std::string &output_path);
		uint8_t *fixImageFormatForStorage(void *image_data, size_t pixel_count, VkFormat originalFormat);

		virtual void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t swapchain_image_idx, bool present);
		void recordBeginCommandBuffer(VkCommandBuffer commandBuffer);
		void recordRenderToImage(VkCommandBuffer commandBuffer);
		void recordCopyToSwapchain(VkCommandBuffer commandBuffer, uint32_t swapchain_image_index);
		void recordEndCommandBuffer(VkCommandBuffer commandBuffer);

		virtual void initProperties();
		void initSceneSelectionProperty() const;

	};

} // namespace RtEngine
#endif // BASICS_VULKANENGINE_HPP
