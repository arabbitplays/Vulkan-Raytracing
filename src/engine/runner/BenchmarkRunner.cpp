#include "BenchmarkRunner.hpp"
#include <omp.h>

#include "ImageUtil.hpp"
#include "PathUtil.hpp"
#include "ReferenceRunner.hpp"

namespace RtEngine {
	constexpr std::string SAMPLE_COUNT_OPTION_NAME = "Sample_Count";
	constexpr std::string REFERENCE_IMAGE_PATH_OPTION_NAME = "Reference_Image";

	BenchmarkRunner::BenchmarkRunner(const std::shared_ptr<EngineContext> &engine_context,
		const std::shared_ptr<GuiManager> &gui_manager, const std::shared_ptr<SceneManager> &scene_manager)
			: Runner(engine_context, gui_manager, scene_manager) {
	}

	void BenchmarkRunner::renderScene() {
		std::shared_ptr<DrawContext> draw_context = createMainDrawContext();
		assert(draw_context->targets.size() == 1);
		std::shared_ptr<RenderTarget> target = draw_context->targets[0];
		target->setSamplesPerFrame(1);

		if (target->getTotalSampleCount() == 0) {
			scene_manager->getCurrentScene()->update();
			draw_context = createMainDrawContext();
		}

		// render one image and then output it if output path is defined
		if (error_calculation_sample_count == static_cast<int32_t>(target->getTotalSampleCount())) {
			// TODO this is timing wise done after the render target is advanced
			renderer->waitForIdle();
			renderer->outputRenderingTarget(target, getTmpImagePath(error_calculation_sample_count));

			if (error_calculation_sample_count == final_sample_count) {
				running = false;
				outputBenchmarkData();
			} else {
				error_calculation_sample_count *= 2;
			}
		}

		drawFrame(draw_context);
	}


	void BenchmarkRunner::drawFrame(const std::shared_ptr<DrawContext> &draw_context) {
		renderer->waitForNextFrameStart();

		VkCommandBuffer cmd = renderer->getNewCommandBuffer();
		std::shared_ptr<RenderTarget> target = draw_context->targets[0];

		uint32_t curr_sample_count = target->getTotalSampleCount();
		bool present_image = error_calculation_sample_count - 1 == curr_sample_count;

		int32_t swapchain_image_idx = 0;
		if (present_image) {
			swapchain_image_idx = renderer->aquireNextSwapchainImage();
			if (swapchain_image_idx < 0) {
				handle_resize();
				return;
			}
		}

		renderer->resetCurrFrameFence();

		prepareFrame(cmd, draw_context);

		renderer->updateRenderTarget(target);
		renderer->recordCommandBuffer(cmd, target, swapchain_image_idx, present_image);

		finishFrame(cmd, draw_context, static_cast<uint32_t>(swapchain_image_idx), present_image);
	}


	void BenchmarkRunner::prepareFrame(VkCommandBuffer cmd, const std::shared_ptr<DrawContext> &draw_context) {
		renderer->updateSceneRepresentation(draw_context, update_flags);
		renderer->recordBeginCommandBuffer(cmd);
	}

	std::string BenchmarkRunner::getTmpImagePath(uint32_t samples) {
		std::string scene_name = PathUtil::getFileName(scene_manager->getCurrentScene()->path);
		return std::format("{}/bm_{}_{}.png", TMP_FOLDER, samples, scene_name);
	}

	std::string BenchmarkRunner::getOutputFilePath() {
		std::string scene_name = PathUtil::getFileName(scene_manager->getCurrentScene()->path);
		return std::format("{}/{}.csv", OUT_FOLDER, scene_name);
	}

	void BenchmarkRunner::outputBenchmarkData() {
		std::string scene_name = PathUtil::getFileName(scene_manager->getCurrentScene()->path);
		std::string ref_path = std::format("{}/8388608_{}.png", REF_FOLDER, scene_name);

		int ref_width, ref_height;
		uint8_t* ref_data = ImageUtil::loadPNG(ref_path, &ref_width, &ref_height);

		assert(ref_data != nullptr);

		std::ofstream out(getOutputFilePath());
		if (!out)
			throw std::runtime_error("Failed to open CSV file");
		out << "samples,mse\n";

		for (uint32_t i = 1; i <= final_sample_count; i *= 2) {
			int width, height;
			uint8_t* data = ImageUtil::loadPNG(getTmpImagePath(i), &width, &height);

			assert(ref_width == width && ref_height == height);
			assert(data != nullptr);

			float mse = calculateMSE(ref_data, data, width * height * 4);
			out << std::format("{},{}\n", i, mse);

			stbi_image_free(data);
		}
		stbi_image_free(ref_data);
	}

	float BenchmarkRunner::calculateMSE(uint8_t* ref_data, uint8_t* data, uint32_t size) {
		QuickTimer timer("MSE Calculation");

		float result = 0;
		for (uint32_t i = 0; i < size; i++) {
			int32_t difference = static_cast<int32_t>(ref_data[i]) - static_cast<int32_t>(data[i]);
			result += static_cast<float>(difference * difference) / static_cast<float>(size);
		}

		return result;
	}

	/*void BenchmarkRunner::initProperties() {
		VulkanRenderer::initProperties();
		renderer_properties->addInt(SAMPLE_COUNT_OPTION_NAME, &sample_count);
		renderer_properties->addString(REFERENCE_IMAGE_PATH_OPTION_NAME, &reference_image_path);
	}*/
} // namespace RtEngine
