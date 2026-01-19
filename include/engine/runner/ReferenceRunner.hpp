#ifndef REFERENCERENDERER_HPP
#define REFERENCERENDERER_HPP

#include "Runner.hpp"
#include "spdlog/stopwatch.h"

namespace RtEngine {
	class ReferenceRunner : public Runner {
		void renderScene() override;
		void drawFrame(std::shared_ptr<DrawContext> draw_context) override;
		//void initProperties() override;

		spdlog::stopwatch stopwatch;
		uint32_t present_sample_count = 1;
		int32_t sample_count = 1000;
		bool present_image = false;
	};

} // namespace RtEngine
#endif // REFERENCERENDERER_HPP
