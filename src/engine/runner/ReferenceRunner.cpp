#include "ReferenceRunner.hpp"

namespace RtEngine {
	constexpr std::string SAMPLE_COUNT_OPTION_NAME = "Sample_Count";

	void ReferenceRunner::mainLoop() {
		loadScene();

		stopwatch.reset();

		while (!glfwWindowShouldClose(window)) {
			// render one image and then output it if output path is defined
			if (sample_count == mainDrawContext->target->getTotalSampleCount()) {
				vkDeviceWaitIdle(vulkan_context->device_manager->getDevice());
				outputRenderingTarget(vulkan_context->base_options->resources_dir + "/references/" +
									  std::to_string(sample_count) + "_render.png");
				break;
			}

			glfwPollEvents();

			drawFrame();
		}

		vkDeviceWaitIdle(vulkan_context->device_manager->getDevice());
	}

	void ReferenceRunner::drawFrame() {
		vkWaitForFences(vulkan_context->device_manager->getDevice(), 1, &inFlightFences[mainDrawContext->currentFrame], VK_TRUE,
						UINT64_MAX);

		uint32_t curr_sample_count = mainDrawContext->target->getTotalSampleCount();
		present_image = present_sample_count == curr_sample_count;

		int imageIndex = 0;
		if (present_image) {
			imageIndex = aquireNextSwapchainImage();
			if (imageIndex < 0)
				return;
		}

		vkResetFences(vulkan_context->device_manager->getDevice(), 1, &inFlightFences[mainDrawContext->currentFrame]);

		scene_manager->updateScene(mainDrawContext);

		vkResetCommandBuffer(commandBuffers[mainDrawContext->currentFrame], 0);
		recordCommandBuffer(commandBuffers[mainDrawContext->currentFrame], imageIndex);

		if (present_image) {
			std::vector<VkSemaphore> waitSemaphore = {imageAvailableSemaphores[mainDrawContext->currentFrame]};
			std::vector<VkSemaphore> signalSemaphore = {renderFinishedSemaphores[mainDrawContext->currentFrame]};
			submitCommandBuffer(waitSemaphore, signalSemaphore);
			presentSwapchainImage(signalSemaphore, imageIndex);

			present_sample_count *= 2;
		} else {
			submitCommandBuffer({}, {});
		}

		if (curr_sample_count % 1000 == 0) {
			double elapsed_time = stopwatch.elapsed().count();
			stopwatch.reset();
			uint32_t samples_left = sample_count - curr_sample_count;
			double time_left = elapsed_time / 1000 * samples_left;
			int hours = static_cast<int>(time_left) / 3600;
			int minutes = (static_cast<int>(time_left) % 3600) / 60;
			int sec = static_cast<int>(time_left) % 60;
			uint32_t progress = round((float) curr_sample_count / (float) sample_count * 100);
			spdlog::info("Current sample count: {}, progress: {}%, estimated time remaining: {}h {}m {}s",
						 curr_sample_count, progress, hours, minutes, sec);
		}

		mainDrawContext->nextFrame();
	}

	void ReferenceRunner::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
		recordBeginCommandBuffer(commandBuffer);
		recordRenderToImage(commandBuffer);
		if (present_image)
			recordCopyToSwapchain(commandBuffer, imageIndex);
		recordEndCommandBuffer(commandBuffer);
	}

	void ReferenceRunner::initProperties() {
		VulkanRenderer::initProperties();
		renderer_properties->addInt(SAMPLE_COUNT_OPTION_NAME, &sample_count);
	}
} // namespace RtEngine
