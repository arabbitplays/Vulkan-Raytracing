#ifndef REFERENCERENDERER_HPP
#define REFERENCERENDERER_HPP

#include <VulkanEngine.hpp>
#include "spdlog/stopwatch.h"

namespace RtEngine {
	class ReferenceRenderer : public VulkanEngine {
		void mainLoop() override;
		void drawFrame() override;
		void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) override;
		void initProperties() override;

		spdlog::stopwatch stopwatch;
		uint32_t present_sample_count = 1;
		int32_t sample_count = 1000;
		bool present_image = false;
	};

} // namespace RtEngine
#endif // REFERENCERENDERER_HPP
