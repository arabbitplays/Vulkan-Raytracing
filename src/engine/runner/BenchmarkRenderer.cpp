#include "BenchmarkRenderer.hpp"
#include <omp.h>

namespace RtEngine {
	constexpr std::string SAMPLE_COUNT_OPTION_NAME = "Sample_Count";
	constexpr std::string REFERENCE_IMAGE_PATH_OPTION_NAME = "Reference_Image";

	void BenchmarkRenderer::mainLoop() {
		omp_set_num_threads(omp_get_max_threads());

		loadScene();
		reference_image_data = vulkan_context->resource_builder->loadImageData(reference_image_path, &ref_width,
																			   &ref_height, &ref_channels);

		uint32_t curr_sample_count = mainDrawContext->target->getTotalSampleCount();

		while (!glfwWindowShouldClose(window)) {
			// render one image and then output it if output path is defined
			if (sample_count == curr_sample_count) {
				vkDeviceWaitIdle(vulkan_context->device_manager->getDevice());
				outputRenderingTarget(std::to_string(sample_count) + "_benchmark.png");
				break;
			}

			glfwPollEvents();

			drawFrame();

			if (calculate_error) {
				calculate_error = false;
				float mse = calculateMSEToReference();
				spdlog::info("Sample count: {}, Error: {:f}", curr_sample_count, mse);
			}
		}

		vkDeviceWaitIdle(vulkan_context->device_manager->getDevice());

		float mse = calculateMSEToReference();
		spdlog::info("Sample count: {}, Final Error: {:f}", curr_sample_count, mse);
		stbi_image_free(reference_image_data);
	}

	void BenchmarkRenderer::drawFrame() {
		vkWaitForFences(vulkan_context->device_manager->getDevice(), 1, &inFlightFences[mainDrawContext->currentFrame], VK_TRUE,
						UINT64_MAX);

		uint32_t curr_sample_count = mainDrawContext->target->getTotalSampleCount();
		if (error_calculation_sample_count == curr_sample_count) {
			present_image = true;
			calculate_error = true;
			error_calculation_sample_count *= 2;
		}

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
		} else {
			submitCommandBuffer({}, {});
		}

		mainDrawContext->nextFrame();
	}

	void BenchmarkRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
		recordBeginCommandBuffer(commandBuffer);
		recordRenderToImage(commandBuffer);
		if (present_image)
			recordCopyToSwapchain(commandBuffer, imageIndex);
		recordEndCommandBuffer(commandBuffer);
	}

	float BenchmarkRenderer::calculateMSEToReference() {
		QuickTimer timer("MSE Calculation");
		AllocatedImage render_target = mainDrawContext->target->getCurrentTargetImage();
		uint32_t width = render_target.imageExtent.width;
		uint32_t height = render_target.imageExtent.height;

		assert(ref_width == width && ref_height == height && ref_channels == 4);

		void *raw_data = vulkan_context->resource_builder->downloadImage(render_target, sizeof(uint32_t));
		uint8_t *image_data = fixImageFormatForStorage(raw_data, width * height, render_target.imageFormat);

		assert(image_data != nullptr && reference_image_data != nullptr);

		std::vector<double> row_sum(height);
		uint32_t row_length = width * 4;
#pragma omp parallel for
		for (uint32_t i = 0; i < height; i++) {
			row_sum[i] = 0;
			for (uint32_t j = 0; j < width * 4; j++) {
				const int32_t difference = image_data[i * row_length + j] - reference_image_data[i * row_length + j];
				row_sum[i] += difference * difference;
			}
		}

		double sum = 0.0f;
#pragma omp parallel for reduction(+ : sum)
		for (int i = 0; i < height; i++) {
			sum += row_sum[i] / static_cast<double>(row_length);
		}

		delete image_data;
		return static_cast<float>(sum / static_cast<double>(height));
	}

	void BenchmarkRenderer::initProperties() {
		VulkanEngine::initProperties();
		renderer_properties->addInt(SAMPLE_COUNT_OPTION_NAME, &sample_count);
		renderer_properties->addString(REFERENCE_IMAGE_PATH_OPTION_NAME, &reference_image_path);
	}
} // namespace RtEngine
