#ifndef VULKAN_RAYTRACING_RENDERER_HPP
#define VULKAN_RAYTRACING_RENDERER_HPP
#include "vulkan_scene_representation/GeometryManager.hpp"
#include "Scene.hpp"

namespace RtEngine {
    class VulkanRenderer {
    public:
        VulkanRenderer(std::shared_ptr<VulkanContext> vulkan_context);


    private:

        std::shared_ptr<VulkanContext> vulkan_context;

    };
}



#endif //VULKAN_RAYTRACING_RENDERER_HPP