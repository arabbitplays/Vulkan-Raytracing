#include "ReferenceRenderer.hpp"

namespace RtEngine {
	constexpr std::string SAMPLE_COUNT_OPTION_NAME = "Sample_Count";

	void ReferenceRenderer::mainLoop() {
		loadScene();

		stopwatch.reset();

		while (!glfwWindowShouldClose(window)) {
			// render one image and then output it if output path is defined
			if (sample_count == properties_manager->curr_sample_count) {
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

	void ReferenceRenderer::drawFrame() {
		vkWaitForFences(vulkan_context->device_manager->getDevice(), 1, &inFlightFences[currentFrame], VK_TRUE,
						UINT64_MAX);

		uint32_t curr_sample_count = properties_manager->curr_sample_count;
		present_image = present_sample_count == curr_sample_count;

		int imageIndex = 0;
		if (present_image) {
			imageIndex = aquireNextSwapchainImage();
			if (imageIndex < 0)
				return;
		}

		vkResetFences(vulkan_context->device_manager->getDevice(), 1, &inFlightFences[currentFrame]);

		scene_manager->updateScene(mainDrawContext, currentFrame, getRenderTarget(), getRngTexture());
		properties_manager->emitting_instances_count =
				scene_manager->getEmittingInstancesCount(); // TODO move this together with the creation of the instance
															// buffers

		vkResetCommandBuffer(commandBuffers[currentFrame], 0);
		recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

		if (present_image) {
			std::vector<VkSemaphore> waitSemaphore = {imageAvailableSemaphores[currentFrame]};
			std::vector<VkSemaphore> signalSemaphore = {renderFinishedSemaphores[currentFrame]};
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

		currentFrame = (currentFrame + 1) % max_frames_in_flight;
	}

	void ReferenceRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
		recordBeginCommandBuffer(commandBuffer);
		recordRenderToImage(commandBuffer);
		if (present_image)
			recordCopyToSwapchain(commandBuffer, imageIndex);
		recordEndCommandBuffer(commandBuffer);
	}

	void ReferenceRenderer::initProperties() {
		VulkanEngine::initProperties();
		renderer_properties->addInt(SAMPLE_COUNT_OPTION_NAME, &sample_count);
	}
} // namespace RtEngine
