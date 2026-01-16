//
// Created by oschdi on 5/4/25.
//

#ifndef REALTIMERENDERER_HPP
#define REALTIMERENDERER_HPP

#include <VulkanEngine.hpp>

namespace RtEngine {
	class RealtimeRenderer : public VulkanEngine {
	public:
		RealtimeRenderer() { max_frames_in_flight = 2; };

	protected:
		void mainLoop() override;
	};
} // namespace RtEngine

#endif // REALTIMERENDERER_HPP
