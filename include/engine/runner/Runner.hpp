//
// Created by oschdi on 18.01.26.
//

#ifndef VULKAN_RAYTRACING_RUNNER_HPP
#define VULKAN_RAYTRACING_RUNNER_HPP
#include "VulkanRenderer.hpp"

namespace RtEngine {
    class Runner {
    public:
        Runner(std::shared_ptr<VulkanRenderer> renderer);

        void loadScene();
        void renderScene();
    private:
        std::shared_ptr<VulkanRenderer> renderer;

        std::shared_ptr<Scene> scene;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_RUNNER_HPP