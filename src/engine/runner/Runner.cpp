//
// Created by oschdi on 18.01.26.
//

#include "../../../include/engine/runner/Runner.hpp"

namespace RtEngine {
    Runner::Runner(std::shared_ptr<VulkanRenderer> renderer) : renderer(renderer) {
        scene_reader = std::make_shared<SceneReader>(renderer->getRuntimeContext());
    }

    std::string Runner::getScenePath() const {
        return scene != nullptr ? scene->path : "";
    }

    void Runner::loadScene(const std::string &scene_path) {
        assert(!scene_path.empty());

        scene = scene_reader->readScene(scene_path, renderer->getMaterials());
        renderer->loadScene(scene);
        //SceneWriter writer;
        //writer.writeScene(PathUtil::getFileName(scene_manager->getSceneInformation().path), scene_manager->scene);
    }

    void Runner::renderScene() {
        drawFrame();
    }

    void Runner::drawFrame() {
        renderer->waitForNextFrameStart();

        const int32_t swapchain_image_idx = renderer->aquireNextSwapchainImage();
        if (swapchain_image_idx < 0)
            return;

        renderer->resetCurrFrame();

        renderer->update();
        renderer->recordCommands(swapchain_image_idx);

        renderer->submitCommands(true, swapchain_image_idx);
    }


} // RtEngine