#ifndef BENCHMARKRENDERER_HPP
#define BENCHMARKRENDERER_HPP
#include <../rendering/renderer/RaytracingRenderer.hpp>

#include "Runner.hpp"

namespace RtEngine {
	class BenchmarkRunner : public Runner {
	public:
		BenchmarkRunner(const std::shared_ptr<EngineContext> &engine_context, const std::shared_ptr<SceneManager> &scene_manager);

		void loadScene(const std::string &scene_path) override;
		void renderScene() override;

		void finishRound();

		void drawFrame(const std::shared_ptr<DrawContext> &draw_context) override;

		void resetForNextRound(std::shared_ptr<RenderTargetRepository> target_repository);

		void initProperties(const std::shared_ptr<IProperties> &config, const UpdateFlagsHandle &update_flags) override;

	private:
		void prepareFrame(VkCommandBuffer cmd, const std::shared_ptr<DrawContext> &draw_context) override;

		std::string getTmpImagePath(uint32_t biased_samples, uint32_t diff_samples);
		std::string getOutputFilePath();
		std::string getRefFilePath();
		void calculateErrorBetweenImages(uint8_t* ref_data, uint32_t ref_width, uint32_t ref_height, uint8_t* data,
		                                  uint32_t width, uint32_t height, uint32_t samples);
		void clearTmpfolder();

		void calculateErrors();

		void outputErrorsToCsv();

		float calculateMSE(uint8_t *ref_data, uint8_t *data, uint32_t size);

		std::string TMP_FOLDER = "./tmp";
		std::string OUT_FOLDER = "../resources/benchmarks";
		std::string REF_FOLDER = "../resources/references";

		std::shared_ptr<DrawContext> draw_context;

		uint32_t expected_ref_sample_count = 1 << 18;

		std::string benchmark_name = "";

		uint32_t error_calculation_frame_count = 1;

		uint32_t final_biased_sample_count = 1 << 10;
		uint32_t final_diff_sample_count = 1 << 10;
		uint32_t biased_samples_per_diff_sample = 0;
		uint32_t averaging_rounds = 1;
		uint32_t done_rounds = 0;

		uint32_t rendered_frame_count = 0;

		bool calculating_mlmc_diff = false;

		std::unordered_map<uint32_t, float> mse_averages{};

		double mean_biased_frame_time = 0;
		double mean_diff_frame_time = 0;
		double mean_combined_frame_time = 0;
	};

} // namespace RtEngine
#endif // BENCHMARKRENDERER_HPP
