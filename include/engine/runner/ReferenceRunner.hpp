#ifndef REFERENCERENDERER_HPP
#define REFERENCERENDERER_HPP

#include "Runner.hpp"
#include "spdlog/stopwatch.h"

namespace RtEngine {
	class ReferenceRunner : public Runner {
	public:
		ReferenceRunner(const std::shared_ptr<EngineContext> &engine_context, const std::shared_ptr<SceneManager> &scene_manager);

		void loadScene(const std::string &scene_path) override;
		void renderScene() override;
		void drawFrame(const std::shared_ptr<DrawContext> &draw_context) override;

	private:
		void prepareFrame(VkCommandBuffer cmd, const std::shared_ptr<DrawContext> &draw_context) override;

		void mergeImages(uint32_t width, uint32_t height);

		std::string getTmpImagePath(uint32_t image_idx, uint32_t samples);
		std::string getOutputImagePath(uint32_t samples);

		float *calculateMean(float *imgA, float *imgB, uint32_t size);

		const std::string TMP_FOLDER = "./tmp";
		const std::string OUT_FOLDER = "../resources/references";

		std::shared_ptr<DrawContext> draw_context;

		spdlog::stopwatch stopwatch;
		uint32_t present_sample_count = 8;
		int32_t final_sample_count = 1 << 20;
		int32_t samples_per_image = 1 << 12;
		// int32_t samples_per_image = 32;
		// int32_t final_sample_count = 256;

		std::vector<float*> done_images;
		uint32_t final_image_count;
	};

} // namespace RtEngine
#endif // REFERENCERENDERER_HPP
