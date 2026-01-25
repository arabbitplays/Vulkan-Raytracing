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

#include <VulkanContext.hpp>
#include "DeletionQueue.hpp"
#include "MeshRepository.hpp"
#include "../Window.hpp"

namespace RtEngine {

	class VulkanRenderer {
	public:
		VulkanRenderer() = default;
		virtual ~VulkanRenderer() = default;
		void init(const std::shared_ptr<BaseOptions> &base_options, std::shared_ptr<Window> window);

		void loadScene(std::shared_ptr<IScene> scene);

		void updateSceneRepresentation(const std::shared_ptr<DrawContext> &draw_context, uint32_t update_flags);
		void updateRenderTarget(const std::shared_ptr<RenderTarget> &target);

		void waitForIdle();
		void waitForNextFrameStart();
		void resetCurrFrameFence();

		int32_t aquireNextSwapchainImage();
		VkCommandBuffer getNewCommandBuffer();
		void recordBeginCommandBuffer(VkCommandBuffer commandBuffer);
		virtual void recordCommandBuffer(VkCommandBuffer commandBuffer, std::shared_ptr<RenderTarget> target, uint32_t swapchain_image_idx, bool present);
		void recordEndCommandBuffer(VkCommandBuffer commandBuffer);
		bool submitCommands(bool present, uint32_t swapchain_image_idx);

		void nextFrame();

		void cleanup();

		void outputRenderingTarget(const std::shared_ptr<RenderTarget> &target, const std::string &output_path);
		float *downloadRenderTarget(const std::shared_ptr<RenderTarget> &target) const;
		uint8_t *fixImageFormatForStorage(void *image_data, size_t pixel_count, VkFormat originalFormat);

		std::shared_ptr<RenderTarget> createRenderTarget(uint32_t width, uint32_t height);

		std::shared_ptr<VulkanContext> getVulkanContext();

		std::shared_ptr<TextureRepository> getTextureRepository();
		std::shared_ptr<MeshRepository> getMeshRepository();
		std::unordered_map<std::string, std::shared_ptr<Material>> getMaterials() const;
		std::shared_ptr<Swapchain> getSwapchain();

		std::shared_ptr<PropertiesManager> getPropertiesManager();

	protected:
		std::shared_ptr<Window> window;

		DeletionQueue mainDeletionQueue;

		std::shared_ptr<BaseOptions> base_options;

		std::shared_ptr<VulkanContext> vulkan_context;
		std::shared_ptr<TextureRepository> texture_repository;
		std::shared_ptr<MeshRepository> mesh_repository;

		std::vector<VkCommandBuffer> commandBuffers;

		std::shared_ptr<PropertiesManager> properties_manager;
		std::shared_ptr<PropertiesSection> renderer_properties;

		std::shared_ptr<SceneAdapter> scene_adapter;

		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		std::vector<VkFence> inFlightFences;

		bool framebufferResized = false;

		uint32_t max_frames_in_flight = 1;
		uint32_t current_frame;

		void initWindow();
		void initVulkan();
		void createVulkanContext();
		void createRepositories();

		static bool hasStencilComponent(VkFormat format);

		std::shared_ptr<DescriptorAllocator> createDescriptorAllocator();
		void createCommandBuffers();
		void createSyncObjects();

		void pollSdlEvents();

		void submitCommandBuffer(const std::vector<VkSemaphore> &wait_semaphore, const std::vector<VkSemaphore> &signal_semaphore);
		void presentSwapchainImage(const std::vector<VkSemaphore>& wait_semaphore, uint32_t image_index);

		void recordRenderToImage(VkCommandBuffer commandBuffer, std::shared_ptr<RenderTarget> target);
		void recordBlitToSwapchain(VkCommandBuffer commandBuffer, const std::shared_ptr<RenderTarget> &render_target, uint32_t swapchain_image_index);

		virtual void initProperties();
	};

} // namespace RtEngine
#endif // BASICS_VULKANENGINE_HPP
