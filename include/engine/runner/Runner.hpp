//
// Created by oschdi on 18.01.26.
//

#ifndef VULKAN_RAYTRACING_RUNNER_HPP
#define VULKAN_RAYTRACING_RUNNER_HPP
#include "SceneManager.hpp"
#include "SceneReader.hpp"
#include "VulkanRenderer.hpp"

namespace RtEngine {
    class Runner {
    public:
        Runner(std::shared_ptr<VulkanRenderer> renderer);

        std::string getScenePath() const;
        void loadScene(const std::string &scene_path);
        virtual void renderScene();

        std::shared_ptr<SceneManager> getSceneManager();
    protected:
        virtual void drawFrame();

        std::shared_ptr<VulkanRenderer> renderer;
        std::shared_ptr<SceneReader> scene_reader;

        std::shared_ptr<SceneManager> scene_manager;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_RUNNER_HPP