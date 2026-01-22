#ifndef REFERENCERENDERER_HPP
#define REFERENCERENDERER_HPP

#include "Runner.hpp"
#include "spdlog/stopwatch.h"

namespace RtEngine {
	class ReferenceRunner : public Runner {
	public:
		ReferenceRunner(const std::shared_ptr<EngineContext> &engine_context, const std::shared_ptr<GuiManager> &gui_manager, const std::shared_ptr<SceneManager> &scene_manager);

		void renderScene() override;
		void drawFrame(const std::shared_ptr<DrawContext> &draw_context) override;
		//void initProperties() override;

	private:
		void prepareFrame(VkCommandBuffer cmd, const std::shared_ptr<DrawContext> &draw_context) override;

		void mergeImages();

		void clearTmpfolder();

		std::string getTmpImagePath(uint32_t image_idx, uint32_t samples);

		std::string getOutputImagePath(uint32_t samples);

		void writeMeanPng(std::string path1, std::string path2, std::string out_path);

		const std::string TMP_FOLDER = "./tmp";
		const std::string OUT_FOLDER = "../resources/references";

		spdlog::stopwatch stopwatch;
		uint32_t present_sample_count = 8;
		int32_t final_sample_count = 1 << 20;
		//int32_t final_sample_count = 32;

		uint32_t done_images = 0;
		uint32_t final_image_count = 8;
	};

} // namespace RtEngine
#endif // REFERENCERENDERER_HPP
