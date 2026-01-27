//
// Created by oschdi on 18.01.26.
//

#include "SceneManager.hpp"

#include <filesystem>

namespace RtEngine {
    SceneManager::SceneManager(const std::string &resources_dir) : resources_dir(resources_dir) {
        std::string scenes_dir = std::format("{}/scenes", resources_dir);
        try {
            for (const auto &entry: std::filesystem::directory_iterator(scenes_dir)) {
                scene_names.push_back(entry.path().filename().stem());
            }
        } catch (const std::exception &e) {
            throw std::runtime_error("failed to load scene directory: " + std::string(e.what()));
        }

        if (scene_names.empty()) {
            throw std::runtime_error("No scenes found in scene directory " + scenes_dir);
        }
    }

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

    std::string SceneManager::getScenePath(std::string scene_name) {
        return std::format("{}/scenes/{}.yaml", resources_dir, scene_name);
    }

    std::vector<std::string> SceneManager::getSceneNames() const {
        return scene_names;
    }
} // RtEngine