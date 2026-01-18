//
// Created by oschdi on 18.01.26.
//

#ifndef VULKAN_RAYTRACING_SCENEMANAGER_HPP
#define VULKAN_RAYTRACING_SCENEMANAGER_HPP
#include <memory>

#include "Scene.hpp"

namespace RtEngine {
    class SceneManager {
    public:
        std::shared_ptr<Scene> scene;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_SCENEMANAGER_HPP