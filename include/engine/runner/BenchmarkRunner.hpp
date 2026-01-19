#ifndef BENCHMARKRENDERER_HPP
#define BENCHMARKRENDERER_HPP
#include <../renderer/VulkanRenderer.hpp>

namespace RtEngine {
	class BenchmarkRunner : public VulkanRenderer {
		void recordCommandBuffer(VkCommandBuffer commandBuffer, std::shared_ptr<RenderTarget> target, uint32_t imageIndex, bool present) override;

		float calculateMSEToReference(std::shared_ptr<RenderTarget> render_target);
		void initProperties() override;

		uint8_t *reference_image_data;
		int ref_width, ref_height, ref_channels;

		uint32_t error_calculation_sample_count = 1;
		bool present_image = false, calculate_error = true;

		std::string reference_image_path;
		int32_t sample_count = 1000;
	};

} // namespace RtEngine
#endif // BENCHMARKRENDERER_HPP
