//
// Created by oschdi on 18.01.26.
//

#include "SceneManager.hpp"

namespace RtEngine {
    std::shared_ptr<Scene> SceneManager::getCurrentScene() {
        return scene;
    }

    void SceneManager::setScene(const std::shared_ptr<Scene> &new_scene) {
        scene = new_scene;
    }

    std::shared_ptr<Material> SceneManager::getCurrentMaterial() {
        if (scene != nullptr) {
            return scene->getMaterial();
        }
        return nullptr;
    }
} // RtEngine