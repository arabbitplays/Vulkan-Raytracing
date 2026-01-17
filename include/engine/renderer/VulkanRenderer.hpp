#ifndef VULKAN_RAYTRACING_RENDERER_HPP
#define VULKAN_RAYTRACING_RENDERER_HPP
#include "GeometryManager.hpp"
#include "Scene.hpp"

namespace RtEngine {
    class VulkanRenderer {
    public:
        VulkanRenderer(std::shared_ptr<VulkanContext> vulkan_context);

        void updateStaticGeometry(std::shared_ptr<Scene> scene);

        void clearGeometry();

    private:

        std::shared_ptr<VulkanContext> vulkan_context;
        std::shared_ptr<GeometryManager> geometry_manager;

    };
}



#endif //VULKAN_RAYTRACING_RENDERER_HPP