#include "BenchmarkRunner.hpp"

#include <filesystem>
#include <omp.h>

#include "ImageUtil.hpp"
#include "PathUtil.hpp"
#include "ReferenceRunner.hpp"

namespace RtEngine {
	constexpr std::string SAMPLE_COUNT_OPTION_NAME = "Sample_Count";
	constexpr std::string REFERENCE_IMAGE_PATH_OPTION_NAME = "Reference_Image";

	BenchmarkRunner::BenchmarkRunner(const std::shared_ptr<EngineContext> &engine_context, const std::shared_ptr<SceneManager> &scene_manager)
			: Runner(engine_context, scene_manager) {

		if (std::filesystem::create_directories(TMP_FOLDER)) {
			SPDLOG_INFO("Created directory {}");
		}

		if (std::filesystem::create_directories(OUT_FOLDER)) {
			SPDLOG_INFO("Created directory {}");
		}
	}

	void BenchmarkRunner::loadScene(const std::string &scene_path) {
		Runner::loadScene(scene_path);

		scene_manager->getCurrentScene()->update();
		draw_context = createMainDrawContext();

		assert(draw_context->targets.size() == 1);
		std::shared_ptr<RenderTarget> target = draw_context->targets[0];
		target->setSamplesPerFrame(1);
	}

	void BenchmarkRunner::renderScene() {
		if (update_flags->checkFlag(SCENE_UPDATE)) {
			loadScene(scene_manager->getScenePath(scene_name));
		}
		std::shared_ptr<RenderTarget> target = draw_context->targets[0];

		// render one image and then output it if output path is defined
		if (error_calculation_sample_count == static_cast<int32_t>(target->getTotalSampleCount())) {
			raytracing_renderer->waitForIdle();
			raytracing_renderer->outputRenderingTarget(target, getTmpImagePath(error_calculation_sample_count));

			if (error_calculation_sample_count == final_sample_count) {
				running = false;
				outputBenchmarkDataToCsv();
			} else {
				error_calculation_sample_count *= 2;
			}
		}

		drawFrame(draw_context);
	}


	void BenchmarkRunner::drawFrame(const std::shared_ptr<DrawContext> &draw_context) {
		raytracing_renderer->waitForNextFrameStart();

		VkCommandBuffer cmd = raytracing_renderer->getNewCommandBuffer();
		std::shared_ptr<RenderTarget> target = draw_context->targets[0];

		uint32_t curr_sample_count = target->getTotalSampleCount();
		bool present_image = error_calculation_sample_count - 1 == curr_sample_count;

		int32_t swapchain_image_idx = 0;
		if (present_image) {
			swapchain_image_idx = raytracing_renderer->aquireNextSwapchainImage();
			if (swapchain_image_idx < 0) {
				handle_resize();
				return;
			}
		}

		prepareFrame(cmd, draw_context);

		raytracing_renderer->updateRenderTarget(target);
		raytracing_renderer->recordCommandBuffer(cmd, target, swapchain_image_idx, present_image);

		finishFrame(cmd, draw_context, static_cast<uint32_t>(swapchain_image_idx), present_image);
	}


	void BenchmarkRunner::prepareFrame(VkCommandBuffer cmd, const std::shared_ptr<DrawContext> &draw_context) {
		raytracing_renderer->updateSceneRepresentation(draw_context, update_flags);
		engine_context->rendering_manager->recordBeginCommandBuffer(cmd);
		update_flags->resetFlags();
	}

	std::string BenchmarkRunner::getTmpImagePath(uint32_t samples) {
		std::string scene_name = PathUtil::getFileName(scene_manager->getCurrentScene()->path);
		return std::format("{}/bm_{}_{}.png", TMP_FOLDER, samples, scene_name);
	}

	std::string BenchmarkRunner::getOutputFilePath() {
		std::string scene_name = PathUtil::getFileName(scene_manager->getCurrentScene()->path);
		return std::format("{}/bm_out.csv", OUT_FOLDER, scene_name);
	}

	std::string BenchmarkRunner::getRefFilePath() {
		std::string scene_name = PathUtil::getFileName(scene_manager->getCurrentScene()->path);
		return std::format("{}/1048576_{}.png", REF_FOLDER, scene_name);
	}

	void BenchmarkRunner::outputBenchmarkDataToCsv() {
		std::string ref_path = getRefFilePath();

		int ref_width, ref_height;
		uint8_t* ref_data = ImageUtil::loadPNG(ref_path, &ref_width, &ref_height);

		assert(ref_data != nullptr);

		std::string output_path = getOutputFilePath();
		std::ofstream out(output_path);
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

		clearTmpfolder();
		SPDLOG_INFO("Saved benchmark data to {}!", output_path);
	}

	void BenchmarkRunner::clearTmpfolder() {
		namespace fs = std::filesystem;
		QuickTimer timer("MSE Calculation");

		if (!fs::exists(TMP_FOLDER) || !fs::is_directory(TMP_FOLDER))
			return;

		for (const fs::directory_entry& entry : fs::directory_iterator(TMP_FOLDER)) {
			if (entry.is_regular_file()) {
				fs::remove(entry.path());
			}
		}
	}

	float BenchmarkRunner::calculateMSE(uint8_t* ref_data, uint8_t* data, uint32_t size) {
		float result = 0;
#pragma omp parallel for reduction(+:result)
		for (uint32_t i = 0; i < size; i++) {
			int32_t difference = static_cast<int32_t>(ref_data[i]) - static_cast<int32_t>(data[i]);
			result += static_cast<float>(difference * difference) / static_cast<float>(size);
		}

		return result;
	}

	/*void BenchmarkRunner::initProperties() {
		RaytracingRenderer::initProperties();
		renderer_properties->addInt(SAMPLE_COUNT_OPTION_NAME, &sample_count);
		renderer_properties->addString(REFERENCE_IMAGE_PATH_OPTION_NAME, &reference_image_path);
	}*/
} // namespace RtEngine
