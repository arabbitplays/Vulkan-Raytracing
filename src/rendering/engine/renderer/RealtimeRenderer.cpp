//
// Created by oschdi on 5/4/25.
//

#include "RealtimeRenderer.hpp"

#include <PathUtil.hpp>

#define SAMPLES_PER_PIXEL 100

namespace RtEngine {
	void RealtimeRenderer::mainLoop() {
		properties_manager->samples_per_pixel = SAMPLES_PER_PIXEL;
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();

			if (PathUtil::getFile(scene_manager->getSceneInformation().path) != vulkan_context->base_options->curr_scene_name) {
				loadScene();
			}
			scene_manager->updateScene(mainDrawContext);
			properties_manager->emitting_instances_count =
					scene_manager->getSceneInformation().emitting_instances_count; // TODO move this together with the creation of the
																// instance buffers

			properties_manager->curr_sample_count = 0;
			drawFrame();
		}

		vkDeviceWaitIdle(vulkan_context->device_manager->getDevice());
	}
} // namespace RtEngine
