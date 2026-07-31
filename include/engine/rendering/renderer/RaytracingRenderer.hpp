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
#include <../targets/RenderTargetRepository.hpp>
#include <../rendering/vulkan_scene_representation/SceneAdapter.hpp>
#include "../../../util/QuickTimer.hpp"
#include "../DescriptorAllocator.hpp"
#include "SceneAdapter.hpp"
#include "VolumeRepository.hpp"

#include <VulkanContext.hpp>
#include "DeletionQueue.hpp"
#include "MeshRepository.hpp"
#include "Renderer.hpp"
#include "UpdateFlagValue.hpp"
#include "../Window.hpp"
#include "renderer/mlmc/MlmcMethod.hpp"
#include "renderer/mlmc/MlmcPresentMode.hpp"

namespace RtEngine {

	class RaytracingRenderer : public ISerializable, public Renderer {
	public:
		RaytracingRenderer(const std::shared_ptr<Window> &window, const std::shared_ptr<VulkanContext> &vulkan_context,
			const std::string &resources_dir, const uint32_t max_frames_in_flight);

		void init() override;
		void initProperties(const std::shared_ptr<IProperties> &config, const UpdateFlagsHandle &update_flags) override;

		void loadScene(std::shared_ptr<IScene> scene);

		void writeResources(const std::shared_ptr<DrawContext> &draw_context, UpdateFlagsHandle update_flags) override;
		void writeRenderTarget(const std::shared_ptr<RenderTargetRepository> &target) override;

		void waitForIdle();

		int32_t aquireNextSwapchainImage();
		virtual void recordCommandBuffer(VkCommandBuffer commandBuffer, std::shared_ptr<RenderTargetRepository> target, uint32_t swapchain_image_idx, bool present);
		bool submitCommands(bool present, uint32_t swapchain_image_idx);

		void cleanup();

		void outputRenderingTarget(const std::shared_ptr<RenderTargetRepository> &target, std::string target_key, const std::string &output_path);
		float *downloadRenderTarget(const std::shared_ptr<RenderTargetRepository> &target, std::string target_key) const;
		uint8_t *fixImageFormatForStorage(void *image_data, size_t pixel_count, VkFormat originalFormat);

		std::shared_ptr<TextureRepository> getTextureRepository();
		std::shared_ptr<MeshRepository> getMeshRepository();
		std::shared_ptr<VolumeRepository> getVolumeRepository();
		std::unordered_map<std::string, std::shared_ptr<Material>> getMaterials() const;

		MlmcPresentMode getMlmcPresentMode();

	protected:
		std::string resources_dir;

		std::shared_ptr<Window> window;

		// Sanity ceiling for the recursion_depth option. Path storage grows
		// dynamically with it: the pipeline is respecialized with a larger
		// MAX_PATH_LENGTH when the depth exceeds the current capacity
		// (Material::ensurePipelineSpecialization), at ~110 B of per-thread scratch
		// per vertex.
		static constexpr uint32_t MAX_RECURSION_DEPTH = 256;

		uint32_t recursion_depth = 5;
		MlmcMethod mlmc_mode = PATH_LENGTH_PREFIX;
		std::string mlmc_mode_str = "path length prefix";
		MlmcPresentMode mlmc_present_mode = COMBINED;
		std::string mlmc_present_mode_str = "combined";

		uint32_t mlmc_biased_path_length = 2;

		std::vector<int32_t> push_constants{};

		std::shared_ptr<TextureRepository> texture_repository;
		std::shared_ptr<MeshRepository> mesh_repository;
		std::shared_ptr<VolumeRepository> volume_repository;

		std::shared_ptr<SceneAdapter> scene_adapter;

		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;

		bool framebufferResized = false;

		void initWindow();
		void createRepositories();

		std::shared_ptr<RenderTargetRepository> createRenderTarget(uint32_t width, uint32_t height);

		std::shared_ptr<DescriptorAllocator> createDescriptorAllocator();

		static bool hasStencilComponent(VkFormat format);

		void createSyncObjects() override;

		void submitCommandBuffer(const std::vector<VkSemaphore> &wait_semaphore, const std::vector<VkSemaphore> &signal_semaphore);
		void presentSwapchainImage(const std::vector<VkSemaphore>& wait_semaphore, uint32_t image_index);

		void recordRenderToImage(VkCommandBuffer commandBuffer, std::shared_ptr<RenderTargetRepository> target);

		void *createPushConstants(uint32_t *size, const std::shared_ptr<RenderTargetRepository> &target);

		void recordBlitToSwapchain(VkCommandBuffer commandBuffer, const AllocatedImage &render_target, uint32_t swapchain_image_index);
	};

} // namespace RtEngine
#endif // BASICS_VULKANENGINE_HPP
