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
        explicit SceneManager(const std::string &resources_dir);

        std::shared_ptr<Scene> getCurrentScene();
        void setScene(const std::shared_ptr<Scene> &new_scene);
        std::shared_ptr<Material> getCurrentMaterial() override;

        std::string getScenePath(std::string scene_name);
        std::vector<std::string> getSceneNames() const;

        void destroy() const;
    private:
        std::string resources_dir;
        std::shared_ptr<Scene> scene;
        std::vector<std::string> scene_names;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_SCENEMANAGER_HPP