#include <HierarchyWindow.hpp>
#include <SceneWriter.hpp>
#include <../../../include/engine/renderer/VulkanRenderer.hpp>
#include <cstdlib>
#include <filesystem>
#include <PathUtil.hpp>
#include <set>
#include <RandomUtil.hpp>

namespace RtEngine {

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

	void CmdTraceRaysKHR(VkDevice device, VkCommandBuffer commandBuffer,
						 const VkStridedDeviceAddressRegionKHR *pRaygenShaderBindingTable,
						 const VkStridedDeviceAddressRegionKHR *pMissShaderBindingTable,
						 const VkStridedDeviceAddressRegionKHR *pHitShaderBindingTable,
						 const VkStridedDeviceAddressRegionKHR *pCallableShaderBindingTable, uint32_t width,
						 uint32_t height, uint32_t depth) {
		auto func = (PFN_vkCmdTraceRaysKHR) vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR");
		if (func != nullptr) {
			return func(commandBuffer, pRaygenShaderBindingTable, pMissShaderBindingTable, pHitShaderBindingTable,
						pCallableShaderBindingTable, width, height, depth);
		}
	}

	void VulkanRenderer::init(const std::shared_ptr<BaseOptions> &base_options, std::shared_ptr<Window> window) {
		this->base_options = base_options;
		this->window = window;
		properties_manager = std::make_shared<PropertiesManager>(base_options->config_file);

		initWindow();
		initVulkan();
	}

	void VulkanRenderer::initWindow() {
		window->addResizeCallback([this](uint32_t width, uint32_t height) {
			framebufferResized = true;
		});
	}

	void VulkanRenderer::initVulkan() {
		createVulkanContext();
		createRepositories();
		createMainDrawContext();

		scene_adapter = std::make_shared<SceneAdapter>(vulkan_context, texture_repository, mainDrawContext->max_frames_in_flight,
													   DeviceManager::RAYTRACING_PROPERTIES);
		mainDeletionQueue.pushFunction([&]() { scene_adapter->clearResources(); });

		createCommandBuffers();
		createSyncObjects();
	}

	void VulkanRenderer::createVulkanContext() {
		vulkan_context = std::make_shared<VulkanContext>();

		vulkan_context->window = window;
		vulkan_context->device_manager = std::make_shared<DeviceManager>(window->getHandle(), enableValidationLayers);

		initProperties();
		properties_manager->addPropertySection(renderer_properties);

		vulkan_context->command_manager = std::make_shared<CommandManager>(vulkan_context->device_manager);
		vulkan_context->resource_builder =
				std::make_shared<ResourceBuilder>(vulkan_context->device_manager, vulkan_context->command_manager,
												  base_options->resources_dir);
		vulkan_context->swapchain =
				std::make_shared<Swapchain>(vulkan_context->device_manager, window->getHandle(), vulkan_context->resource_builder);
		vulkan_context->descriptor_allocator = createDescriptorAllocator();

		mainDeletionQueue.pushFunction([&]() {
			vulkan_context->descriptor_allocator->destroyPools(vulkan_context->device_manager->getDevice());
			vulkan_context->swapchain->destroy();
			vulkan_context->command_manager->destroy();
			vulkan_context->device_manager->destroy();
		});
	}

	void VulkanRenderer::createRepositories() {
		mesh_repository = std::make_shared<MeshRepository>(vulkan_context, base_options->resources_dir);
		texture_repository = std::make_shared<TextureRepository>(vulkan_context->resource_builder);

		mainDeletionQueue.pushFunction([&]() {
			mesh_repository->destroy();
			texture_repository->destroy();
		});
	}

	void VulkanRenderer::createMainDrawContext() {
		mainDrawContext = std::make_shared<DrawContext>();
		mainDrawContext->max_frames_in_flight = max_frames_in_flight;
	}

	std::shared_ptr<RenderTarget> VulkanRenderer::createRenderTarget() {
		return std::make_shared<RenderTarget>(vulkan_context->resource_builder, vulkan_context->swapchain->extent, mainDrawContext->max_frames_in_flight);
	}

	std::shared_ptr<DescriptorAllocator> VulkanRenderer::createDescriptorAllocator() {
		std::vector<DescriptorAllocator::PoolSizeRatio> poolRatios = {
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
				{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
				{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
				{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
				{VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1},
		};

		auto descriptorAllocator = std::make_shared<DescriptorAllocator>();
		descriptorAllocator->init(vulkan_context->device_manager->getDevice(), 8, poolRatios);

		return descriptorAllocator;
	}

	bool VulkanRenderer::hasStencilComponent(const VkFormat format) {
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	void VulkanRenderer::loadScene(std::shared_ptr<IScene> scene) {
		scene_adapter->loadNewScene(scene);
		properties_manager->addPropertySection(scene->getMaterial()->getProperties());
	}

	void VulkanRenderer::createCommandBuffers() {
		commandBuffers.resize(mainDrawContext->max_frames_in_flight);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = vulkan_context->command_manager->commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

		if (vkAllocateCommandBuffers(vulkan_context->device_manager->getDevice(), &allocInfo, commandBuffers.data()) !=
			VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command bufer!");
		}
	}

	void VulkanRenderer::createSyncObjects() {
		imageAvailableSemaphores.resize(mainDrawContext->max_frames_in_flight);
		renderFinishedSemaphores.resize(mainDrawContext->max_frames_in_flight);
		inFlightFences.resize(mainDrawContext->max_frames_in_flight);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < mainDrawContext->max_frames_in_flight; i++) {
			if (vkCreateSemaphore(vulkan_context->device_manager->getDevice(), &semaphoreInfo, nullptr,
								  &imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(vulkan_context->device_manager->getDevice(), &semaphoreInfo, nullptr,
								  &renderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(vulkan_context->device_manager->getDevice(), &fenceInfo, nullptr, &inFlightFences[i]) !=
						VK_SUCCESS) {
				throw std::runtime_error("failed to create sync objects");
			}

			mainDeletionQueue.pushFunction([&, i]() {
				vkDestroySemaphore(vulkan_context->device_manager->getDevice(), imageAvailableSemaphores[i], nullptr);
				vkDestroySemaphore(vulkan_context->device_manager->getDevice(), renderFinishedSemaphores[i], nullptr);
				vkDestroyFence(vulkan_context->device_manager->getDevice(), inFlightFences[i], nullptr);
			});
		}
	}

	void VulkanRenderer::update() {
		scene_adapter->updateScene(mainDrawContext);
	}

	void VulkanRenderer::waitForNextFrameStart() {
		vkWaitForFences(vulkan_context->device_manager->getDevice(), 1, &inFlightFences[mainDrawContext->currentFrame], VK_TRUE,
						UINT64_MAX);
	}

	int32_t VulkanRenderer::aquireNextSwapchainImage() {
		uint32_t imageIndex;
		VkResult result =
				vkAcquireNextImageKHR(vulkan_context->device_manager->getDevice(), vulkan_context->swapchain->handle,
									  UINT64_MAX, imageAvailableSemaphores[mainDrawContext->currentFrame], VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			refreshAfterResize();
			return -1;
		} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}
		return static_cast<int32_t>(imageIndex);
	}

	void VulkanRenderer::resetCurrFrame() {
		vkResetFences(vulkan_context->device_manager->getDevice(), 1, &inFlightFences[mainDrawContext->currentFrame]);
	}

	bool VulkanRenderer::submitCommands(bool present, int32_t swapchain_image_idx) {
		if (present) {
			std::vector<VkSemaphore> waitSemaphore = {imageAvailableSemaphores[mainDrawContext->currentFrame]};
			std::vector<VkSemaphore> signalSemaphore = {renderFinishedSemaphores[mainDrawContext->currentFrame]};
			submitCommandBuffer(waitSemaphore, signalSemaphore);
			presentSwapchainImage(signalSemaphore, swapchain_image_idx);
		} else {
			submitCommandBuffer({} , {});
		}

		mainDrawContext->nextFrame();

		return framebufferResized;
	}

	void VulkanRenderer::submitCommandBuffer(std::vector<VkSemaphore> wait_semaphore,
	                                         std::vector<VkSemaphore> signal_semaphore) {
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submitInfo.waitSemaphoreCount = static_cast<uint32_t>(wait_semaphore.size());
		submitInfo.pWaitSemaphores = wait_semaphore.data();
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[mainDrawContext->currentFrame];
		submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signal_semaphore.size());
		;
		submitInfo.pSignalSemaphores = signal_semaphore.data();

		if (vkQueueSubmit(vulkan_context->device_manager->getQueue(GRAPHICS), 1, &submitInfo,
						  inFlightFences[mainDrawContext->currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}
	}

	void VulkanRenderer::presentSwapchainImage(const std::vector<VkSemaphore>& wait_semaphore, const uint32_t image_index) {
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = static_cast<uint32_t>(wait_semaphore.size());
		presentInfo.pWaitSemaphores = wait_semaphore.data();
		VkSwapchainKHR swapChains[] = {vulkan_context->swapchain->handle};
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &image_index;

		VkResult result = vkQueuePresentKHR(vulkan_context->device_manager->getQueue(PRESENT), &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
			framebufferResized = true;
		} else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}
	}

	void VulkanRenderer::waitForIdle() {
		vkDeviceWaitIdle(vulkan_context->device_manager->getDevice());
	}

	void VulkanRenderer::refreshAfterResize() {
		framebufferResized = false;
		mainDrawContext->target->resetAccumulatedFrames();
		vulkan_context->swapchain->recreate();
		mainDrawContext->target->recreate(vulkan_context->swapchain->extent);
	}

	VkCommandBuffer VulkanRenderer::getNewCommandBuffer() {
		vkResetCommandBuffer(commandBuffers[mainDrawContext->currentFrame], 0);
		return commandBuffers[mainDrawContext->currentFrame];
	}

	void VulkanRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, const uint32_t swapchain_image_idx, bool present) {
		recordRenderToImage(commandBuffer);
		if (present) {
			recordCopyToSwapchain(commandBuffer, swapchain_image_idx);
		}
	}

	void VulkanRenderer::recordBeginCommandBuffer(VkCommandBuffer commandBuffer) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin record command buffer!");
		}
	}

	void VulkanRenderer::recordRenderToImage(VkCommandBuffer commandBuffer) {
		Pipeline pipeline = *scene_adapter->getMaterial()->pipeline;

		const uint32_t handleSizeAligned =
				VulkanUtil::alignedSize(DeviceManager::RAYTRACING_PROPERTIES.shaderGroupHandleSize,
										DeviceManager::RAYTRACING_PROPERTIES.shaderGroupHandleAlignment);

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
		descriptor_sets.push_back(scene_adapter->getSceneDescriptorSet(mainDrawContext->currentFrame));
		descriptor_sets.push_back(scene_adapter->getMaterial()->materialDescriptorSet);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline.getHandle());
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline.getLayoutHandle(), 0,
								static_cast<uint32_t>(descriptor_sets.size()), descriptor_sets.data(), 0, nullptr);

		uint32_t pc_size;
		void *pc_data = properties_manager->getPushConstants(&pc_size, mainDrawContext->target);
		vkCmdPushConstants(commandBuffer, pipeline.getLayoutHandle(),
						   VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_KHR |
								   VK_SHADER_STAGE_MISS_BIT_KHR,
						   0, pc_size, pc_data);

		CmdTraceRaysKHR(vulkan_context->device_manager->getDevice(), commandBuffer, &raygenShaderSbtEntry,
						&missShaderSbtEntry, &closestHitShaderSbtEntry, &callableShaderSbtEntry,
						vulkan_context->swapchain->extent.width, vulkan_context->swapchain->extent.height, 1);
	}

	void VulkanRenderer::recordCopyToSwapchain(VkCommandBuffer commandBuffer, const uint32_t swapchain_image_index) {
		AllocatedImage render_target = mainDrawContext->target->getCurrentTargetImage();
		std::shared_ptr<ResourceBuilder> resource_builder = vulkan_context->resource_builder;
		std::shared_ptr<Swapchain> swapchain = vulkan_context->swapchain;

		resource_builder->transitionImageLayout(commandBuffer, swapchain->images[swapchain_image_index],
												VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
												VK_ACCESS_NONE, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
												VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		resource_builder->transitionImageLayout(
				commandBuffer, render_target.image, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
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
					   swapchain->images[swapchain_image_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion,
					   VK_FILTER_NEAREST);

		resource_builder->transitionImageLayout(commandBuffer, swapchain->images[swapchain_image_index],
												VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
												VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_NONE,
												VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		resource_builder->transitionImageLayout(commandBuffer, render_target.image, VK_PIPELINE_STAGE_TRANSFER_BIT,
												VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
												VK_ACCESS_NONE, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
												VK_IMAGE_LAYOUT_GENERAL);
	}

	void VulkanRenderer::recordEndCommandBuffer(VkCommandBuffer commandBuffer) {
		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}

	void VulkanRenderer::cleanup() {
		mainDeletionQueue.flush();
		window->destroy();
	}

	void VulkanRenderer::outputRenderingTarget(const std::string &output_path) {
		AllocatedImage render_target = mainDrawContext->target->getCurrentTargetImage();
		void *data = vulkan_context->resource_builder->downloadImage(render_target, sizeof(uint32_t));
		uint8_t *fixed_data = fixImageFormatForStorage(
				data, render_target.imageExtent.width * render_target.imageExtent.height, render_target.imageFormat);
		vulkan_context->resource_builder->writePNG(output_path, fixed_data, render_target.imageExtent.width,
												   render_target.imageExtent.height);

		delete fixed_data;
	}

	// target format is R8G8B8A8_UNORM
	uint8_t *VulkanRenderer::fixImageFormatForStorage(void *data, size_t pixel_count, VkFormat originalFormat) {
		if (originalFormat == VK_FORMAT_R8G8B8A8_UNORM)
			return static_cast<uint8_t *>(data);

		if (originalFormat == VK_FORMAT_B8G8R8A8_UNORM) {
			auto image_data = static_cast<uint8_t *>(data);
			for (size_t i = 0; i < pixel_count; i++) {
				std::swap(image_data[i * 4], image_data[i * 4 + 2]); // Swap B (0) and R (2)
			}
			return image_data;
		}
		if (originalFormat == VK_FORMAT_R32G32B32A32_SFLOAT) {
			uint8_t *output_image = new uint8_t[pixel_count * 4];
			auto image_data = static_cast<float *>(data);
			for (size_t i = 0; i < pixel_count * 4; i++) {
				float test = image_data[i];
				// Clamp each channel to the [0, 1] range and then scale to [0, 255]
				output_image[i] = static_cast<uint8_t>(std::fmin(1.0f, std::fmax(0.0f, image_data[i])) * 255);
			}
			delete image_data;
			return output_image;
		} else {
			spdlog::error("Image format of the storage image is not supported to be stored correctly!");
			return nullptr;
		}
	}

	void VulkanRenderer::initProperties() {
		renderer_properties = std::make_shared<PropertiesSection>(RENDERER_SECTION_NAME);

		renderer_properties->addString(RESOURCES_DIR_OPTION_NAME, &base_options->resources_dir);
		renderer_properties->addInt(RECURSION_DEPTH_OPTION_NAME, &base_options->max_depth, 1, 5);

		initSceneSelectionProperty();
	}

	void VulkanRenderer::initSceneSelectionProperty() const
	{
		assert(renderer_properties != nullptr);
		std::string scenes_dir = base_options->resources_dir + "/scenes";
		std::vector<std::string> scenes;
		try {
			for (const auto &entry: std::filesystem::__cxx11::directory_iterator(scenes_dir)) {
				scenes.push_back(entry.path().filename());
			}
		} catch (const std::exception &e) {
			throw std::runtime_error("failed to load scene directory: " + std::string(e.what()));
		}

		if (scenes.empty()) {
			throw std::runtime_error("No scenes found in scene directory " + scenes_dir + ".");
		}
		base_options->curr_scene_name = scenes[0];

		renderer_properties->addSelection(CURR_SCENE_OPTION_NAME, &base_options->curr_scene_name,
										  scenes);
	}

	void VulkanRenderer::handleGuiUpdate(uint32_t update_flags) const
	{
		// TODO this in runner
		mainDrawContext->target->resetAccumulatedFrames();
		scene_adapter->bufferUpdateFlags |= update_flags;
	}

	std::shared_ptr<VulkanContext> VulkanRenderer::getVulkanContext() {
		return vulkan_context;
	}

	std::shared_ptr<TextureRepository> VulkanRenderer::getTextureRepository() {
		return texture_repository;
	}

	std::shared_ptr<MeshRepository> VulkanRenderer::getMeshRepository() {
		return mesh_repository;
	}

	std::unordered_map<std::string, std::shared_ptr<Material>> VulkanRenderer::getMaterials() const {
		return scene_adapter->defaultMaterials;
	}

	std::shared_ptr<RenderTarget> VulkanRenderer::getRenderTarget() const {
		return mainDrawContext->target;
	}

	std::shared_ptr<PropertiesManager> VulkanRenderer::getPropertiesManager() {
		return properties_manager;
	}

	VkExtent2D VulkanRenderer::getSwapchainExtent() {
		return vulkan_context->swapchain->extent;
	}
} // namespace RtEngine
