#ifndef VULKAN_RAYTRACING_RENDERINGMANAGER_HPP
#define VULKAN_RAYTRACING_RENDERINGMANAGER_HPP
#include <memory>

#include "VulkanContext.hpp"
#include "RaytracingRenderer.hpp"

namespace RtEngine {
    class RenderingManager {
    public:
        RenderingManager() = default;
        RenderingManager(const std::shared_ptr<Window> &window, std::string resources_dir,
                         bool enable_validation_layer);

        void initRendererProperties(const std::shared_ptr<IProperties> &properties, const std::shared_ptr<UpdateFlags> &update_flags);

        std::shared_ptr<VulkanContext> getVulkanContext() const;
        std::shared_ptr<RaytracingRenderer> getRaytracingRenderer() const;

        void destroy();
    private:
        void createVulkanContext();
        std::shared_ptr<DescriptorAllocator> createDescriptorAllocator() const;

        void createRenderer();

        std::shared_ptr<Window> window;
        bool validation_layers_enabled;
        std::string resources_dir;

        DeletionQueue deletion_queue;
        std::shared_ptr<VulkanContext> vulkan_context;

        std::shared_ptr<RaytracingRenderer> raytracing_renderer;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_RENDERINGMANAGER_HPP