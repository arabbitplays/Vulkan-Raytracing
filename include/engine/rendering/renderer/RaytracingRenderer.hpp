#ifndef BASICS_VULKANENGINE_HPP
#define BASICS_VULKANENGINE_HPP

#include <vector>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <chrono>
#define TINYOBJLOADER_IMPLEMENTATION
#include <AccelerationStructure.hpp>
#include <GuiRenderer.hpp>
#include <GuiWindow.hpp>
#include <memory>
#include <RenderTarget.hpp>
#include <../rendering/vulkan_scene_representation/SceneAdapter.hpp>
#include "../../../util/QuickTimer.hpp"
#include "../DescriptorAllocator.hpp"
#include "SceneAdapter.hpp"

#include <VulkanContext.hpp>
#include "DeletionQueue.hpp"
#include "MeshRepository.hpp"
#include "Renderer.hpp"
#include "UpdateFlagValue.hpp"
#include "../../Window.hpp"

namespace RtEngine {

	class RaytracingRenderer : public ISerializable, public Renderer {
	public:
		RaytracingRenderer(const std::shared_ptr<Window> &window, const std::shared_ptr<VulkanContext> &vulkan_context,
			const std::string &resources_dir, const uint32_t max_frames_in_flight);

		void init() override;
		void initProperties(const std::shared_ptr<IProperties> &config, const UpdateFlagsHandle &update_flags) override;

		void loadScene(std::shared_ptr<IScene> scene);

		void updateSceneRepresentation(const std::shared_ptr<DrawContext> &draw_context, UpdateFlagsHandle update_flags);
		void updateRenderTarget(const std::shared_ptr<RenderTarget> &target) override;

		void waitForIdle();

		int32_t aquireNextSwapchainImage();
		virtual void recordCommandBuffer(VkCommandBuffer commandBuffer, std::shared_ptr<RenderTarget> target, uint32_t swapchain_image_idx, bool present);
		bool submitCommands(bool present, uint32_t swapchain_image_idx);

		void cleanup();

		void outputRenderingTarget(const std::shared_ptr<RenderTarget> &target, const std::string &output_path);
		float *downloadRenderTarget(const std::shared_ptr<RenderTarget> &target) const;
		uint8_t *fixImageFormatForStorage(void *image_data, size_t pixel_count, VkFormat originalFormat);

		std::shared_ptr<TextureRepository> getTextureRepository();
		std::shared_ptr<MeshRepository> getMeshRepository();
		std::unordered_map<std::string, std::shared_ptr<Material>> getMaterials() const;

	protected:
		std::string resources_dir;

		std::shared_ptr<Window> window;

		uint32_t recursion_depth = 5;
		std::vector<int32_t> push_constants{};

		std::shared_ptr<TextureRepository> texture_repository;
		std::shared_ptr<MeshRepository> mesh_repository;

		std::shared_ptr<SceneAdapter> scene_adapter;

		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;

		bool framebufferResized = false;

		void initWindow();
		void createRepositories();

		static bool hasStencilComponent(VkFormat format);

		void createSyncObjects() override;

		void submitCommandBuffer(const std::vector<VkSemaphore> &wait_semaphore, const std::vector<VkSemaphore> &signal_semaphore);
		void presentSwapchainImage(const std::vector<VkSemaphore>& wait_semaphore, uint32_t image_index);

		void recordRenderToImage(VkCommandBuffer commandBuffer, std::shared_ptr<RenderTarget> target);

		void *createPushConstants(uint32_t *size, const std::shared_ptr<RenderTarget> &target);

		void recordBlitToSwapchain(VkCommandBuffer commandBuffer, const std::shared_ptr<RenderTarget> &render_target, uint32_t swapchain_image_index);
	};

} // namespace RtEngine
#endif // BASICS_VULKANENGINE_HPP
