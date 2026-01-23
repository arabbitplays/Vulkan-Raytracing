#ifndef BENCHMARKRENDERER_HPP
#define BENCHMARKRENDERER_HPP
#include <../renderer/VulkanRenderer.hpp>

#include "Runner.hpp"

namespace RtEngine {
	class BenchmarkRunner : public Runner {
	public:
		BenchmarkRunner(const std::shared_ptr<EngineContext> &engine_context, const std::shared_ptr<GuiManager> &gui_manager, const std::shared_ptr<SceneManager> &scene_manager);

		void renderScene() override;
		void drawFrame(const std::shared_ptr<DrawContext> &draw_context) override;
		//void initProperties() override;

	private:
		void prepareFrame(VkCommandBuffer cmd, const std::shared_ptr<DrawContext> &draw_context) override;

		std::string getTmpImagePath(uint32_t samples);

		std::string getOutputFilePath();

		void outputBenchmarkData();

		float calculateMSE(uint8_t *ref_data, uint8_t *data, uint32_t size);

		std::string TMP_FOLDER = "./tmp";
		std::string OUT_FOLDER = "../resources/benchmarks";
		std::string REF_FOLDER = "../resources/references";

		uint32_t error_calculation_sample_count = 1;
		uint32_t final_sample_count = 256;
	};

} // namespace RtEngine
#endif // BENCHMARKRENDERER_HPP
