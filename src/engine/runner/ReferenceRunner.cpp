#include "ReferenceRunner.hpp"

#include <filesystem>

#include "ImageUtil.hpp"
#include "PathUtil.hpp"
#include <format>

namespace RtEngine {
	constexpr std::string SAMPLE_COUNT_OPTION_NAME = "Sample_Count";

	ReferenceRunner::ReferenceRunner(const std::shared_ptr<EngineContext> &engine_context,
		const std::shared_ptr<GuiManager> &gui_manager, const std::shared_ptr<SceneManager> &scene_manager)
			: Runner(engine_context, gui_manager, scene_manager) {
	}

	void ReferenceRunner::renderScene() {
		std::shared_ptr<DrawContext> draw_context = createMainDrawContext();
		assert(draw_context->targets.size() == 1);
		std::shared_ptr<RenderTarget> target = draw_context->targets[0];
		target->setSamplesPerFrame(8);

		if (done_images == 0 && target->getTotalSampleCount() == 0) {
			scene_manager->getCurrentScene()->update();
			draw_context = createMainDrawContext();
		}

		// render one image and then output it if output path is defined
		if (final_sample_count == static_cast<int32_t>(target->getTotalSampleCount())) {
			// TODO this is timing wise done after the render target is advanced
			renderer->waitForIdle();
			renderer->outputRenderingTarget(target, getTmpImagePath(done_images, final_sample_count));

			done_images++;
			if (done_images == final_image_count) {
				running = false;
				mergeImages();
			} else {
				target->resetAccumulatedFrames();
				present_sample_count = 8;
			}
			return;
		}

		drawFrame(draw_context);
	}


	void ReferenceRunner::drawFrame(const std::shared_ptr<DrawContext> &draw_context) {
		renderer->waitForNextFrameStart();

		VkCommandBuffer cmd = renderer->getNewCommandBuffer();
		std::shared_ptr<RenderTarget> target = draw_context->targets[0];

		uint32_t curr_sample_count = target->getTotalSampleCount();
		bool present_image = present_sample_count <= curr_sample_count;

		int32_t swapchain_image_idx = 0;
		if (present_image) {
			swapchain_image_idx = renderer->aquireNextSwapchainImage();
			if (swapchain_image_idx < 0) {
				handle_resize();
				return;
			}
			present_sample_count *= 2;
		}

		renderer->resetCurrFrameFence();

		prepareFrame(cmd, draw_context);

		renderer->updateRenderTarget(target);
		renderer->recordCommandBuffer(cmd, target, swapchain_image_idx, present_image);

		finishFrame(cmd, draw_context, static_cast<uint32_t>(swapchain_image_idx), present_image);

		if (curr_sample_count % 1000 == 0) {
			double elapsed_time = stopwatch.elapsed().count();
			stopwatch.reset();

			uint32_t total_sample_count = final_image_count * final_sample_count;
			uint32_t collected_sample_count = done_images * final_sample_count + curr_sample_count;

			uint32_t samples_left = total_sample_count - collected_sample_count;
			double time_left = elapsed_time / 1000.0f * samples_left;
			int hours = static_cast<int>(time_left) / 3600;
			int minutes = (static_cast<int>(time_left) % 3600) / 60;
			int sec = static_cast<int>(time_left) % 60;
			uint32_t progress = round(static_cast<float>(collected_sample_count) / static_cast<float>(total_sample_count) * 100.0f);
			SPDLOG_INFO("Collected sample count: {}, progress: {}%, estimated time remaining: {}h {}m {}s",
						 collected_sample_count, progress, hours, minutes, sec);
		}
	}


	void ReferenceRunner::prepareFrame(VkCommandBuffer cmd, const std::shared_ptr<DrawContext> &draw_context) {
		renderer->updateSceneRepresentation(draw_context, update_flags);
		renderer->recordBeginCommandBuffer(cmd);
	}

	void ReferenceRunner::mergeImages() {
		uint32_t current_sample_count = final_sample_count;
		uint32_t current_image_count = final_image_count;
		while (current_image_count != 1) {
			for (uint32_t i = 0; i < current_image_count; i += 2) {
				std::string path1 = getTmpImagePath(i, current_sample_count);
				std::string path2 = getTmpImagePath(i + 1, current_sample_count);
				std::string out_path = current_sample_count * 2 == final_image_count * final_sample_count
					? getOutputImagePath(final_image_count * final_sample_count)
					: getTmpImagePath(i / 2, current_sample_count * 2);
				writeMeanPng(path1, path2, out_path);
			}
			current_image_count /= 2;
			current_sample_count *= 2;
		}

		//clearTmpfolder();
	}

	void ReferenceRunner::clearTmpfolder() {
		namespace fs = std::filesystem;

		if (!fs::exists(TMP_FOLDER) || !fs::is_directory(TMP_FOLDER))
			return;

		for (const fs::directory_entry& entry : fs::directory_iterator(TMP_FOLDER)) {
			if (entry.is_regular_file()) {
				fs::remove(entry.path());
			}
		}
	}

	std::string ReferenceRunner::getTmpImagePath(uint32_t image_idx, uint32_t samples) {
		std::string scene_name = PathUtil::getFileName(scene_manager->getCurrentScene()->path);
		return std::format("{}/{}_{}_{}.png", TMP_FOLDER, samples, scene_name, image_idx);
	}

	std::string ReferenceRunner::getOutputImagePath(uint32_t samples) {
		std::string scene_name = PathUtil::getFileName(scene_manager->getCurrentScene()->path);
		return std::format("{}/{}_{}.png", OUT_FOLDER, samples, scene_name);
	}

	void ReferenceRunner::writeMeanPng(std::string path1, std::string path2, std::string out_path) {
		int w1, h1;
		int w2, h2;

		uint8_t* imgA = ImageUtil::loadPNG(path1, &w1, &h1);
		uint8_t* imgB = ImageUtil::loadPNG(path2, &w2, &h2);

		if (w1 != w2 || h1 != h2) {
			stbi_image_free(imgA);
			stbi_image_free(imgB);
			throw std::runtime_error("PNG dimensions do not match");
		}

		const int count = w1 * h1 * 4;
		std::vector<uint8_t> out(count);

		for (int i = 0; i < count; ++i) {
			out[i] = static_cast<unsigned char>(
				(static_cast<uint32_t>(imgA[i]) + static_cast<uint32_t>(imgB[i])) / 2
			);
		}

		ImageUtil::writePNG(out_path, out.data(), w1, h1);
	}


	/*void ReferenceRunner::initProperties() {
		VulkanRenderer::initProperties();
		renderer_properties->addInt(SAMPLE_COUNT_OPTION_NAME, &sample_count);
	}*/
} // namespace RtEngine
