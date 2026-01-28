#include "ReferenceRunner.hpp"

#include <filesystem>

#include "ImageUtil.hpp"
#include "PathUtil.hpp"
#include <format>

namespace RtEngine {
	constexpr std::string SAMPLE_COUNT_OPTION_NAME = "Sample_Count";

	ReferenceRunner::ReferenceRunner(const std::shared_ptr<EngineContext> &engine_context, const std::shared_ptr<SceneManager> &scene_manager)
			: Runner(engine_context, scene_manager) {
		final_image_count = final_sample_count / samples_per_image;
		if (std::filesystem::create_directories(OUT_FOLDER)) {
			SPDLOG_INFO("Created directory {}");
		}
	}

	void ReferenceRunner::loadScene(const std::string &scene_path) {
		Runner::loadScene(scene_path);

		scene_manager->getCurrentScene()->update();
		draw_context = createMainDrawContext();

		assert(draw_context->targets.size() == 1);
		std::shared_ptr<RenderTarget> target = draw_context->targets[0];
		target->setSamplesPerFrame(8);

		stopwatch.reset();
	}

	void ReferenceRunner::renderScene() {
		if (update_flags->checkFlag(SCENE_UPDATE)) {
			loadScene(scene_manager->getScenePath(scene_name));
		}
		std::shared_ptr<RenderTarget> target = draw_context->targets[0];

		if (samples_per_image == static_cast<int32_t>(target->getTotalSampleCount())) {
			raytracing_renderer->waitForIdle();

			float *data = raytracing_renderer->downloadRenderTarget(target);
			done_images.push_back(data);

			if (done_images.size() == final_image_count) {
				running = false;
				mergeImages(target->getExtent().width, target->getExtent().height);
			} else {
				target->resetAccumulatedFrames();
				present_sample_count = 8;
			}
			return;
		}

		drawFrame(draw_context);
	}


	void ReferenceRunner::drawFrame(const std::shared_ptr<DrawContext> &draw_context) {
		raytracing_renderer->waitForNextFrameStart();

		VkCommandBuffer cmd = raytracing_renderer->getNewCommandBuffer();
		std::shared_ptr<RenderTarget> target = draw_context->targets[0];

		uint32_t curr_sample_count = target->getTotalSampleCount();
		bool present_image = present_sample_count <= curr_sample_count;

		int32_t swapchain_image_idx = 0;
		if (present_image) {
			swapchain_image_idx = raytracing_renderer->aquireNextSwapchainImage();
			if (swapchain_image_idx < 0) {
				handle_resize();
				return;
			}
			present_sample_count *= 2;
		}

		raytracing_renderer->resetCurrFrameFence();

		prepareFrame(cmd, draw_context);

		raytracing_renderer->updateRenderTarget(target);
		raytracing_renderer->recordCommandBuffer(cmd, target, swapchain_image_idx, present_image);

		finishFrame(cmd, draw_context, static_cast<uint32_t>(swapchain_image_idx), present_image);

		if (curr_sample_count % (1 << 10) == 0) {
			double elapsed_time = stopwatch.elapsed().count();
			uint32_t collected_sample_count = done_images.size() * samples_per_image + curr_sample_count;

			uint32_t samples_left = final_sample_count - collected_sample_count;
			double time_left = elapsed_time / collected_sample_count * samples_left;
			int hours = static_cast<int>(time_left) / 3600;
			int minutes = (static_cast<int>(time_left) % 3600) / 60;
			int sec = static_cast<int>(time_left) % 60;
			uint32_t progress = round(static_cast<float>(collected_sample_count) / static_cast<float>(final_sample_count) * 100.0f);
			SPDLOG_INFO("Collected sample count: {}, progress: {}%, estimated time remaining: {}h {}m {}s",
						 collected_sample_count, progress, hours, minutes, sec);
		}
	}


	void ReferenceRunner::prepareFrame(VkCommandBuffer cmd, const std::shared_ptr<DrawContext> &draw_context) {
		raytracing_renderer->updateSceneRepresentation(draw_context, update_flags);
		raytracing_renderer->recordBeginCommandBuffer(cmd);
		update_flags->resetFlags();
	}

	void ReferenceRunner::mergeImages(const uint32_t width, const uint32_t height) {
		std::vector<float*> merged_images{};
		while (done_images.size() != 1) {
			assert(done_images.size() % 2 == 0);
			for (uint32_t i = 0; i < done_images.size(); i += 2) {
				merged_images.push_back(calculateMean(done_images[i], done_images[i+1], width * height * 4));
			}
			done_images = merged_images;
			merged_images.clear();
		}

		uint8_t *fixed_data = raytracing_renderer->fixImageFormatForStorage(
			done_images[0], width * height, VK_FORMAT_R32G32B32A32_SFLOAT);
		ImageUtil::writePNG(getOutputImagePath(final_sample_count), fixed_data, width, height);

		delete[] fixed_data;
		done_images.clear(); // every pointer is now invalid anyway
	}

	std::string ReferenceRunner::getTmpImagePath(uint32_t image_idx, uint32_t samples) {
		std::string scene_name = PathUtil::getFileName(scene_manager->getCurrentScene()->path);
		return std::format("{}/{}_{}_{}.png", TMP_FOLDER, samples, scene_name, image_idx);
	}

	std::string ReferenceRunner::getOutputImagePath(uint32_t samples) {
		std::string scene_name = PathUtil::getFileName(scene_manager->getCurrentScene()->path);
		return std::format("{}/{}_{}.png", OUT_FOLDER, samples, scene_name);
	}

	float* ReferenceRunner::calculateMean(float* imgA, float* imgB, uint32_t size) {
		for (int i = 0; i < size; ++i) {
			imgA[i] = (imgA[i] + imgB[i]) / 2.0f;
		}

		delete[] imgB;
		return imgA;
	}


	/*void ReferenceRunner::initProperties() {
		RaytracingRenderer::initProperties();
		renderer_properties->addInt(SAMPLE_COUNT_OPTION_NAME, &sample_count);
	}*/
} // namespace RtEngine
