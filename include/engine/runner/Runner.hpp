//
// Created by oschdi on 18.01.26.
//

#ifndef VULKAN_RAYTRACING_RUNNER_HPP
#define VULKAN_RAYTRACING_RUNNER_HPP
#include "SceneReader.hpp"
#include "VulkanRenderer.hpp"

namespace RtEngine {
    class Runner {
    public:
        Runner(std::shared_ptr<VulkanRenderer> renderer);

        std::string getScenePath() const;
        void loadScene(const std::string &scene_path);
        virtual void renderScene();
        virtual void drawFrame();
    private:
        std::shared_ptr<VulkanRenderer> renderer;
        std::shared_ptr<SceneReader> scene_reader;

        std::shared_ptr<Scene> scene;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_RUNNER_HPP