#include <HierarchyWindow.hpp>
#include <cstdlib>
#include <filesystem>
#include <RandomUtil.hpp>
#include <glm/gtc/packing.hpp>

#include "ImageUtil.hpp"
#include "UpdateFlagValue.hpp"
#include "targets/RenderTargetKeys.hpp"

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
		: Renderer(vulkan_context, max_frames_in_flight), resources_dir(resources_dir), window(window) {
	}

	void RaytracingRenderer::init() {
		Renderer::init();

		initWindow();

		createRepositories();

		scene_adapter = std::make_shared<SceneAdapter>(vulkan_context, texture_repository, max_frames_in_flight,
													   DeviceManager::RAYTRACING_PROPERTIES);
		deletion_queue.pushFunction([&]() { scene_adapter->clearResources(); });

	}

	void RaytracingRenderer::initWindow() {
		window->addResizeCallback([this](uint32_t width, uint32_t height) {
			framebufferResized = true;
		});
	}

	void RaytracingRenderer::createRepositories() {
		mesh_repository = std::make_shared<MeshRepository>(vulkan_context, resources_dir);
		texture_repository = std::make_shared<TextureRepository>(vulkan_context->resource_builder);
		volume_repository = std::make_shared<VolumeRepository>(vulkan_context, resources_dir);

		deletion_queue.pushFunction([&]() {
			mesh_repository->destroy();
			texture_repository->destroy();
			volume_repository->destroy();
		});
	}

	std::shared_ptr<RenderTargetRepository> RaytracingRenderer::createRenderTarget(uint32_t width, uint32_t height) {
		VkExtent2D extent(width, height);
		return std::make_shared<RenderTargetRepository>(vulkan_context->resource_builder, extent, max_frames_in_flight);
	}

	std::shared_ptr<DescriptorAllocator> RaytracingRenderer::createDescriptorAllocator() {
		std::vector<DescriptorAllocator::PoolSizeRatio> poolRatios = {
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
				{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
				{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4},
				{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1},
				{VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1},
		};

        auto descriptorAllocator = std::make_shared<DescriptorAllocator>();
        descriptorAllocator->init(vulkan_context->device_manager->getDevice(), 8, poolRatios);

        return descriptorAllocator;
    }

	bool RaytracingRenderer::hasStencilComponent(const VkFormat format) {
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	void RaytracingRenderer::loadScene(std::shared_ptr<IScene> scene) {
		scene_adapter->loadNewScene(scene);
	}

	void RaytracingRenderer::createSyncObjects() {
		Renderer::createSyncObjects();

		uint32_t swapchain_image_count = vulkan_context->swapchain->images.size();

		imageAvailableSemaphores.resize(max_frames_in_flight);
		renderFinishedSemaphores.resize(swapchain_image_count);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < max_frames_in_flight; i++) {
			if (vkCreateSemaphore(vulkan_context->device_manager->getDevice(), &semaphoreInfo, nullptr,
								  &imageAvailableSemaphores[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create sync objects");
			}

			deletion_queue.pushFunction([&, i]() {
				vkDestroySemaphore(vulkan_context->device_manager->getDevice(), imageAvailableSemaphores[i], nullptr);
			});
		}

        for (size_t i = 0; i < swapchain_image_count; i++) {
            if (vkCreateSemaphore(vulkan_context->device_manager->getDevice(), &semaphoreInfo, nullptr,
                                  &renderFinishedSemaphores[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create sync objects");
            }

			deletion_queue.pushFunction([&, i]() {
				vkDestroySemaphore(vulkan_context->device_manager->getDevice(), renderFinishedSemaphores[i], nullptr);
			});
		}
	}

	void RaytracingRenderer::writeResources(const std::shared_ptr<DrawContext> &draw_context, UpdateFlagsHandle update_flags) {
		scene_adapter->updateScene(draw_context, current_frame, update_flags);
	}

	void RaytracingRenderer::writeRenderTarget(const std::shared_ptr<RenderTargetRepository> &target) {
		scene_adapter->updateRenderTarget(target);
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

	void RaytracingRenderer::submitCommandBuffer(const std::vector<VkSemaphore> &wait_semaphore,
	                                         const std::vector<VkSemaphore> &signal_semaphore) {
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submitInfo.waitSemaphoreCount = static_cast<uint32_t>(wait_semaphore.size());
		submitInfo.pWaitSemaphores = wait_semaphore.data();
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &command_buffers[current_frame];
		submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signal_semaphore.size());
		;
		submitInfo.pSignalSemaphores = signal_semaphore.data();

		if (vkQueueSubmit(vulkan_context->device_manager->getQueue(GRAPHICS), 1, &submitInfo,
						  in_flight_fences[current_frame]) != VK_SUCCESS) {
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

	AllocatedImage getPresentTarget(std::shared_ptr<RenderTargetRepository> &target_repository, MlmcPresentMode present_mode) {
        switch (present_mode) {
            case UNBIASED:
                return target_repository->getCurrRenderTargetImage(MAIN_TARGET_KEY);
            case BIASED:
                return target_repository->getCurrRenderTargetImage(MAIN_TARGET_KEY);
            case DIFF:
                return target_repository->getCurrRenderTargetImage(DIFF_TARGET_KEY);
            case COMBINED:
                return target_repository->getCurrRenderTargetImage(MLMC_TARGET_KEY);
                break;
        }
    }

    void RaytracingRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, std::shared_ptr<RenderTargetRepository> target,
                                             const uint32_t swapchain_image_idx, bool present) {
        recordRenderToImage(commandBuffer, target);
        if (present) {
            AllocatedImage render_target = getPresentTarget(target, mlmc_present_mode);
            recordBlitToSwapchain(commandBuffer, render_target, swapchain_image_idx);
        }
    }

	void RaytracingRenderer::recordRenderToImage(VkCommandBuffer commandBuffer, std::shared_ptr<RenderTargetRepository> target) {
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

	void* RaytracingRenderer::createPushConstants(uint32_t* size, const std::shared_ptr<RenderTargetRepository> &target) {
		push_constants.clear();

        push_constants.push_back(recursion_depth);
        std::shared_ptr<Material> material = scene_adapter->getMaterial();
        material->getPushConstantValues(push_constants);

        push_constants.push_back(target->getAccumulatedFrameCount());
        push_constants.push_back(target->getAccumulatedDiffFrameCount());
        push_constants.push_back(target->getSamplesPerFrame());
        push_constants.push_back(target->getDiffSamplesPerFrame());

        push_constants.push_back(mlmc_present_mode != UNBIASED);
        push_constants.push_back(MlmcModeConverter::toIndex(mlmc_mode));

        push_constants.push_back(mlmc_biased_path_length);

        *size = sizeof(uint32_t) * push_constants.size();
        return push_constants.data();
    }

    void RaytracingRenderer::recordBlitToSwapchain(VkCommandBuffer commandBuffer,
                                               const AllocatedImage &render_target,
                                               const uint32_t swapchain_image_index) {
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
        const int32_t target_width = render_target.imageExtent.width;
        const int32_t target_height = render_target.imageExtent.height;


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

	void RaytracingRenderer::cleanup() {
		deletion_queue.flush();
	}

	float* RaytracingRenderer::downloadRenderTarget(const std::shared_ptr<RenderTargetRepository> &target, std::string target_key) const {
		AllocatedImage image = target->getLastRenderTargetImage(target_key);
		uint8_t *data = vulkan_context->resource_builder->downloadImage(image, sizeof(float));
		return reinterpret_cast<float*>(data);
	}

	void RaytracingRenderer::outputRenderingTarget(const std::shared_ptr<RenderTargetRepository> &target, std::string target_key, const std::string &output_path) {
		QuickTimer timer("Output render target");

        AllocatedImage render_target = target->getLastRenderTargetImage(target_key);
        uint8_t *data = vulkan_context->resource_builder->downloadImage(render_target, sizeof(float));
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
        bool target_reset = false;
        if (config->startChild("renderer")) {
            target_reset |= config->addUint("recursion_depth", &recursion_depth, 1, 50);
            config->endChild();
        }

        if (config->startChild("mlmc")) {
            if (config->addSelection("mlmc_mode", &mlmc_mode_str, MlmcModeConverter::getSelectionStrings())) {
                mlmc_mode = MlmcModeConverter::fromString(mlmc_mode_str);
                target_reset = true;
            }

            if (config->addSelection("mlmc_present_mode", &mlmc_present_mode_str, MlmcPresentModeConverter::getSelectionStrings())) {
                mlmc_present_mode = MlmcPresentModeConverter::fromString(mlmc_present_mode_str);
                target_reset = true;
            }

            target_reset |= config->addUint("biased_path_length", &mlmc_biased_path_length, 1, 10);
            config->endChild();
        }

        if (target_reset) {
            update_flags->setFlag(TARGET_RESET);
        }

        for (auto [name, material]: scene_adapter->defaultMaterials) {
            material->initProperties(config, update_flags);
        }
    }

	std::shared_ptr<TextureRepository> RaytracingRenderer::getTextureRepository() {
		return texture_repository;
	}

	std::shared_ptr<MeshRepository> RaytracingRenderer::getMeshRepository() {
		return mesh_repository;
	}

	std::shared_ptr<VolumeRepository> RaytracingRenderer::getVolumeRepository() {
		return volume_repository;
	}

	std::unordered_map<std::string, std::shared_ptr<Material>> RaytracingRenderer::getMaterials() const {
		return scene_adapter->defaultMaterials;
	}

	MlmcPresentMode RaytracingRenderer::getMlmcPresentMode() {
		return mlmc_present_mode;
	}
} // namespace RtEngine
