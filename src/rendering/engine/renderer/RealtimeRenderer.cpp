//
// Created by oschdi on 5/4/25.
//

#include "RealtimeRenderer.hpp"

#include <PathUtil.hpp>

namespace RtEngine {
	void RealtimeRenderer::mainLoop() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();

			if (PathUtil::getFile(scene_manager->getSceneInformation().path) != vulkan_context->base_options->curr_scene_name) {
				loadScene();
			}
			scene_manager->updateScene(mainDrawContext);
			scene_manager->getMaterial()->resetSamples();
			drawFrame();
		}

		vkDeviceWaitIdle(vulkan_context->device_manager->getDevice());
	}
} // namespace RtEngine
