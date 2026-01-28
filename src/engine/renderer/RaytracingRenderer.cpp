#include <HierarchyWindow.hpp>
#include <SceneWriter.hpp>
#include <../../../include/engine/renderer/RaytracingRenderer.hpp>
#include <cstdlib>
#include <filesystem>
#include <PathUtil.hpp>
#include <set>
#include <RandomUtil.hpp>
#include <glm/gtc/packing.hpp>

#include "ImageUtil.hpp"
#include "UpdateFlagValue.hpp"

namespace RtEngine {

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

	RaytracingRenderer::RaytracingRenderer(const std::shared_ptr<Window>& window, const std::shared_ptr<VulkanContext> &vulkan_context,
		const std::string &resources_dir, const uint32_t max_frames_in_flight)
		: resources_dir(resources_dir), window(window), vulkan_context(vulkan_context), max_frames_in_flight(max_frames_in_flight) {
		init();
	}

	void RaytracingRenderer::init() {
		initWindow();

		createRepositories();

		scene_adapter = std::make_shared<SceneAdapter>(vulkan_context, texture_repository, max_frames_in_flight,
													   DeviceManager::RAYTRACING_PROPERTIES);
		main_deletion_queue.pushFunction([&]() { scene_adapter->clearResources(); });

		createCommandBuffers();
		createSyncObjects();
	}

	void RaytracingRenderer::initWindow() {
		window->addResizeCallback([this](uint32_t width, uint32_t height) {
			framebufferResized = true;
		});
	}

	void RaytracingRenderer::createRepositories() {
		mesh_repository = std::make_shared<MeshRepository>(vulkan_context, resources_dir);
		texture_repository = std::make_shared<TextureRepository>(vulkan_context->resource_builder);

		main_deletion_queue.pushFunction([&]() {
			mesh_repository->destroy();
			texture_repository->destroy();
		});
	}

	bool RaytracingRenderer::hasStencilComponent(const VkFormat format) {
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	void RaytracingRenderer::loadScene(std::shared_ptr<IScene> scene) {
		scene_adapter->loadNewScene(scene);
	}

	void RaytracingRenderer::createCommandBuffers() {
		commandBuffers.resize(max_frames_in_flight);

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

	void RaytracingRenderer::createSyncObjects() {
		uint32_t swapchain_image_count = vulkan_context->swapchain->images.size();

		imageAvailableSemaphores.resize(max_frames_in_flight);
		renderFinishedSemaphores.resize(swapchain_image_count);
		inFlightFences.resize(max_frames_in_flight);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < max_frames_in_flight; i++) {
			if (vkCreateSemaphore(vulkan_context->device_manager->getDevice(), &semaphoreInfo, nullptr,
								  &imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(vulkan_context->device_manager->getDevice(), &fenceInfo, nullptr, &inFlightFences[i]) !=
						VK_SUCCESS) {
				throw std::runtime_error("failed to create sync objects");
			}

			main_deletion_queue.pushFunction([&, i]() {
				vkDestroySemaphore(vulkan_context->device_manager->getDevice(), imageAvailableSemaphores[i], nullptr);
				vkDestroyFence(vulkan_context->device_manager->getDevice(), inFlightFences[i], nullptr);
			});
		}

		for (size_t i = 0; i < swapchain_image_count; i++) {
			if (vkCreateSemaphore(vulkan_context->device_manager->getDevice(), &semaphoreInfo, nullptr,
								  &renderFinishedSemaphores[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create sync objects");
						}

			main_deletion_queue.pushFunction([&, i]() {
				vkDestroySemaphore(vulkan_context->device_manager->getDevice(), renderFinishedSemaphores[i], nullptr);
			});
		}
	}

	void RaytracingRenderer::updateSceneRepresentation(const std::shared_ptr<DrawContext> &draw_context, UpdateFlagsHandle update_flags) {
		scene_adapter->updateScene(draw_context, current_frame, update_flags);
	}

	void RaytracingRenderer::updateRenderTarget(const std::shared_ptr<RenderTarget> &target) {
		scene_adapter->updateRenderTarget(target);
	}

	void RaytracingRenderer::waitForNextFrameStart() {
		vkWaitForFences(vulkan_context->device_manager->getDevice(), 1, &inFlightFences[current_frame], VK_TRUE,
						UINT64_MAX);
	}

	int32_t RaytracingRenderer::aquireNextSwapchainImage() {
		uint32_t imageIndex;
		VkResult result =
				vkAcquireNextImageKHR(vulkan_context->device_manager->getDevice(), vulkan_context->swapchain->handle,
									  UINT64_MAX, imageAvailableSemaphores[current_frame], VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			return -1;
		} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to acquire swap chain image!");
		}
		return static_cast<int32_t>(imageIndex);
	}

	void RaytracingRenderer::resetCurrFrameFence() {
		vkResetFences(vulkan_context->device_manager->getDevice(), 1, &inFlightFences[current_frame]);
	}

	bool RaytracingRenderer::submitCommands(bool present, uint32_t swapchain_image_idx) {
		if (present) {
			std::vector<VkSemaphore> waitSemaphore = {imageAvailableSemaphores[current_frame]};
			std::vector<VkSemaphore> signalSemaphore = {renderFinishedSemaphores[swapchain_image_idx]};
			submitCommandBuffer(waitSemaphore, signalSemaphore);
			presentSwapchainImage(signalSemaphore, swapchain_image_idx);
		} else {
			submitCommandBuffer({} , {});
		}

		bool rebuild_needed = framebufferResized;
		framebufferResized = false;

		return rebuild_needed;
	}

	void RaytracingRenderer::nextFrame() {
		current_frame = (current_frame + 1) % max_frames_in_flight;
	}

	void RaytracingRenderer::submitCommandBuffer(const std::vector<VkSemaphore> &wait_semaphore,
	                                         const std::vector<VkSemaphore> &signal_semaphore) {
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submitInfo.waitSemaphoreCount = static_cast<uint32_t>(wait_semaphore.size());
		submitInfo.pWaitSemaphores = wait_semaphore.data();
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[current_frame];
		submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signal_semaphore.size());
		;
		submitInfo.pSignalSemaphores = signal_semaphore.data();

		if (vkQueueSubmit(vulkan_context->device_manager->getQueue(GRAPHICS), 1, &submitInfo,
						  inFlightFences[current_frame]) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}
	}

	void RaytracingRenderer::presentSwapchainImage(const std::vector<VkSemaphore>& wait_semaphore, const uint32_t image_index) {
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

	void RaytracingRenderer::waitForIdle() {
		vkDeviceWaitIdle(vulkan_context->device_manager->getDevice());
	}

	VkCommandBuffer RaytracingRenderer::getNewCommandBuffer() {
		vkResetCommandBuffer(commandBuffers[current_frame], 0);
		return commandBuffers[current_frame];
	}

	void RaytracingRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, std::shared_ptr<RenderTarget> target, const uint32_t swapchain_image_idx, bool present) {
		recordRenderToImage(commandBuffer, target);
		if (present) {
			recordBlitToSwapchain(commandBuffer, target, swapchain_image_idx);
		}
	}

	void RaytracingRenderer::recordBeginCommandBuffer(VkCommandBuffer commandBuffer) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin record command buffer!");
		}
	}

	void RaytracingRenderer::recordRenderToImage(VkCommandBuffer commandBuffer, std::shared_ptr<RenderTarget> target) {
		RaytracingPipeline pipeline = *scene_adapter->getMaterial()->pipeline;

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
		descriptor_sets.push_back(scene_adapter->getSceneDescriptorSet(current_frame));
		descriptor_sets.push_back(scene_adapter->getMaterial()->materialDescriptorSet);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline.getHandle());
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline.getLayoutHandle(), 0,
								static_cast<uint32_t>(descriptor_sets.size()), descriptor_sets.data(), 0, nullptr);

		uint32_t pc_size;
		void *pc_data = createPushConstants(&pc_size, target);
		vkCmdPushConstants(commandBuffer, pipeline.getLayoutHandle(),
						   VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_KHR |
								   VK_SHADER_STAGE_MISS_BIT_KHR,
						   0, pc_size, pc_data);

		const auto [width, height] = target->getExtent();
		CmdTraceRaysKHR(vulkan_context->device_manager->getDevice(), commandBuffer, &raygenShaderSbtEntry,
						&missShaderSbtEntry, &closestHitShaderSbtEntry, &callableShaderSbtEntry,
						width, height, 1);
	}

	void* RaytracingRenderer::createPushConstants(uint32_t* size, const std::shared_ptr<RenderTarget> &target) {
		push_constants.clear();

		push_constants.push_back(recursion_depth);
		std::shared_ptr<Material> material = scene_adapter->getMaterial();
		material->getPushConstantValues(push_constants);

		push_constants.push_back(target->getAccumulatedFrameCount());
		push_constants.push_back(target->getSamplesPerFrame());

		*size = sizeof(uint32_t) * push_constants.size();
		return push_constants.data();
	}

	void RaytracingRenderer::recordBlitToSwapchain(VkCommandBuffer commandBuffer, const std::shared_ptr<RenderTarget> &target, const uint32_t swapchain_image_index) {
		AllocatedImage render_target = target->getCurrentTargetImage();
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

		const int32_t swapchain_width = swapchain->extent.width;
		const int32_t swapchain_height = swapchain->extent.height;
		const int32_t target_width = target->getExtent().width;
		const int32_t target_height = target->getExtent().height;

		VkImageBlit blitRegion{};
		blitRegion.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
		blitRegion.srcOffsets[0] = {0, 0, 0};
		blitRegion.srcOffsets[1] = {target_width, target_height, 1};
		blitRegion.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
		blitRegion.dstOffsets[0] = {0, 0, 0};
		blitRegion.dstOffsets[1] = {swapchain_width, swapchain_height, 1};

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

	void RaytracingRenderer::recordEndCommandBuffer(VkCommandBuffer commandBuffer) {
		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}

	void RaytracingRenderer::cleanup() {
		main_deletion_queue.flush();
		window->destroy();
	}

	float* RaytracingRenderer::downloadRenderTarget(const std::shared_ptr<RenderTarget> &target) const {
		AllocatedImage image = target->getLastTargetImage();
		uint8_t *data = vulkan_context->resource_builder->downloadImage(image, sizeof(float));
		return reinterpret_cast<float*>(data);
	}

	void RaytracingRenderer::outputRenderingTarget(const std::shared_ptr<RenderTarget> &target, const std::string &output_path) {
		QuickTimer timer("Output render target");

		AllocatedImage render_target = target->getLastTargetImage();
		uint8_t *data = vulkan_context->resource_builder->downloadImage(render_target, sizeof(uint32_t));
		uint8_t *fixed_data = fixImageFormatForStorage(
				data, render_target.imageExtent.width * render_target.imageExtent.height, render_target.imageFormat);

		ImageUtil::writePNG(output_path, fixed_data, render_target.imageExtent.width, render_target.imageExtent.height);

		delete[] fixed_data;
	}

	// target format is R8G8B8A8_UNORM
	uint8_t *RaytracingRenderer::fixImageFormatForStorage(void *data, size_t pixel_count, VkFormat originalFormat) {

		if (originalFormat == VK_FORMAT_R8G8B8A8_UNORM)
			return static_cast<uint8_t *>(data);

		if (originalFormat == VK_FORMAT_B8G8R8A8_UNORM) {
			auto image_data = static_cast<uint8_t *>(data);
#pragma omp parallel for
			for (size_t i = 0; i < pixel_count; i++) {
				std::swap(image_data[i * 4], image_data[i * 4 + 2]); // Swap B (0) and R (2)
			}
			return image_data;
		}
		if (originalFormat == VK_FORMAT_R32G32B32A32_SFLOAT) {
			uint8_t *output_image = new uint8_t[pixel_count * 4];
			auto image_data = static_cast<float *>(data);

#pragma omp parallel for
			for (size_t i = 0; i < pixel_count * 4; i++) {
				// Clamp each channel to the [0, 1] range and then scale to [0, 255]
				output_image[i] = static_cast<uint8_t>(std::fmin(1.0f, std::fmax(0.0f, image_data[i])) * 255);
			}
			delete[] image_data;
			return output_image;
		} else {
			spdlog::error("Image format of the storage image is not supported to be stored correctly!");
			return nullptr;
		}
	}

	void RaytracingRenderer::initProperties(const std::shared_ptr<IProperties> &config,
	const UpdateFlagsHandle &update_flags) {
		if (config->startChild("renderer")) {
			if (config->addUint("recursion_depth", &recursion_depth, 1, 10)) {
				update_flags->setFlag(TARGET_RESET);
			}
			config->endChild();
		}

		for (auto [name, material] : scene_adapter->defaultMaterials) {
			material->initProperties(config, update_flags);
		}
	}

	std::shared_ptr<TextureRepository> RaytracingRenderer::getTextureRepository() {
		return texture_repository;
	}

	std::shared_ptr<MeshRepository> RaytracingRenderer::getMeshRepository() {
		return mesh_repository;
	}

	std::unordered_map<std::string, std::shared_ptr<Material>> RaytracingRenderer::getMaterials() const {
		return scene_adapter->defaultMaterials;
	}
} // namespace RtEngine
