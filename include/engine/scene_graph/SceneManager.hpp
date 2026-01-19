//
// Created by oschdi on 18.01.26.
//

#ifndef VULKAN_RAYTRACING_SCENEMANAGER_HPP
#define VULKAN_RAYTRACING_SCENEMANAGER_HPP
#include <memory>

#include "Camera.hpp"
#include "ISceneManager.hpp"
#include "Scene.hpp"

namespace RtEngine {
    class SceneManager : public ISceneManager {
    public:
        std::shared_ptr<Scene> getCurrentScene();
        void setScene(const std::shared_ptr<Scene> &new_scene);
        std::shared_ptr<Material> getCurrentMaterial() override;

    private:
        std::shared_ptr<Scene> scene;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_SCENEMANAGER_HPP