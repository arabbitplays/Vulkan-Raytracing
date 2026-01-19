#include "ReferenceRunner.hpp"

namespace RtEngine {
	constexpr std::string SAMPLE_COUNT_OPTION_NAME = "Sample_Count";

	void ReferenceRunner::renderScene() {
		/*stopwatch.reset();

		// render one image and then output it if output path is defined
		if (sample_count == renderer->getRenderTarget()->getTotalSampleCount()) {
			vkDeviceWaitIdle(vulkan_context->device_manager->getDevice());
			outputRenderingTarget(base_options->resources_dir + "/references/" +
								  std::to_string(sample_count) + "_render.png");
			break;
		}

		drawFrame();*/
	}

	void ReferenceRunner::drawFrame(std::shared_ptr<DrawContext> draw_context) {
		/*renderer->waitForNextFrameStart();

		uint32_t curr_sample_count = renderer->getRenderTarget()->getTotalSampleCount();
		present_image = present_sample_count == curr_sample_count;

		int swapchain_image_idx = 0;
		if (present_image) {
			swapchain_image_idx = renderer->aquireNextSwapchainImage();
			if (swapchain_image_idx < 0)
				return;

			present_sample_count *= 2;
		}

		renderer->resetCurrFrame();

		/renderer->update(TODO);
		VkCommandBuffer command_buffer = renderer->getCurrentCommandBuffer();
		renderer->recordBeginCommandBuffer(command_buffer);
		renderer->recordCommandBuffer(command_buffer, true, swapchain_image_idx);
		renderer->recordEndCommandBuffer(command_buffer);
		if (renderer->submitCommands(true, swapchain_image_idx)) {
			// TODO handle resize when rendering references
			renderer->refreshAfterResize();
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
		}*/
	}

	/*void ReferenceRunner::initProperties() {
		VulkanRenderer::initProperties();
		renderer_properties->addInt(SAMPLE_COUNT_OPTION_NAME, &sample_count);
	}*/
} // namespace RtEngine
