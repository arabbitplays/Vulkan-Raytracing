//
// Created by oschdi on 18.01.26.
//

#ifndef VULKAN_RAYTRACING_ENGINE_HPP
#define VULKAN_RAYTRACING_ENGINE_HPP
#include <memory>

namespace RtEngine {
    class Engine {
    public:
        Engine() = default;

        void run();
        void mainLoop();
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_ENGINE_HPP