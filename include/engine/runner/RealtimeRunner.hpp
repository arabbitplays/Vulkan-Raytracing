//
// Created by oschdi on 5/4/25.
//

#ifndef REALTIMERENDERER_HPP
#define REALTIMERENDERER_HPP
#include "Runner.hpp"

namespace RtEngine {
	class RealtimeRunner : public Runner {
	public:
		RealtimeRunner(std::shared_ptr<EngineContext> engine_context, const std::shared_ptr<SceneManager> &scene_manager) : Runner(engine_context, scene_manager) {
			//max_frames_in_flight = 2;
		} ;

	protected:
	};
} // namespace RtEngine

#endif // REALTIMERENDERER_HPP
