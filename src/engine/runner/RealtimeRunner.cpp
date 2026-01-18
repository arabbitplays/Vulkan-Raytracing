//
// Created by oschdi on 5/4/25.
//

#include "RealtimeRunner.hpp"

#include <PathUtil.hpp>

#define SAMPLES_PER_PIXEL 100

namespace RtEngine {
	void RealtimeRunner::mainLoop() {
		mainDrawContext->target->setSamplesPerFrame(SAMPLES_PER_PIXEL);
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();

			if (PathUtil::getFile(scene_manager->getSceneInformation().path) != vulkan_context->base_options->curr_scene_name) {
				loadScene();
			}
			scene_manager->updateScene(mainDrawContext);

			mainDrawContext->target->resetAccumulatedFrames();
			drawFrame();
		}

		vkDeviceWaitIdle(vulkan_context->device_manager->getDevice());
	}
} // namespace RtEngine
