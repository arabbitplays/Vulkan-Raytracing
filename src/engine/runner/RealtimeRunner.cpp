//
// Created by oschdi on 5/4/25.
//

#include "RealtimeRunner.hpp"

#include <PathUtil.hpp>

#define SAMPLES_PER_PIXEL 100

namespace RtEngine {
	/*void RealtimeRunner::mainLoop() {
		mainDrawContext->target->setSamplesPerFrame(SAMPLES_PER_PIXEL);
		while (window->is_open()) {
			window->pollEvents();
			if (PathUtil::getFile(scene_manager->getSceneInformation().path) != base_options->curr_scene_name) {
				loadScene(TODO);
			}
			scene_manager->updateScene(mainDrawContext);

			mainDrawContext->target->resetAccumulatedFrames();
			drawFrame();
		}

		vkDeviceWaitIdle(vulkan_context->device_manager->getDevice());
	}*/
} // namespace RtEngine
