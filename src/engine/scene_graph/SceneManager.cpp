//
// Created by oschdi on 18.01.26.
//

#include "SceneManager.hpp"

namespace RtEngine {

    std::shared_ptr<Material> SceneManager::getCurrentMaterial() {
        if (scene != nullptr) {
            return scene->getMaterial();
        }
        return nullptr;
    }

} // RtEngine