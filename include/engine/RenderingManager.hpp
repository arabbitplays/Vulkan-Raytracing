#ifndef VULKAN_RAYTRACING_RENDERINGMANAGER_HPP
#define VULKAN_RAYTRACING_RENDERINGMANAGER_HPP
#include <memory>

#include "ComputeRenderer.hpp"
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
        std::shared_ptr<GuiRenderer> getGuiRenderer() const;
        std::shared_ptr<ComputeRenderer> getGlitchRenderer() const;

        std::shared_ptr<RenderTarget> createRenderTarget(uint32_t width, uint32_t height);

        void recordBeginCommandBuffer(VkCommandBuffer &commandBuffer);

        void recordEndCommandBuffer(VkCommandBuffer &commandBuffer);

        void destroy();
    private:
        void createVulkanContext();
        std::shared_ptr<DescriptorAllocator> createDescriptorAllocator() const;

        void createRenderer();

        std::shared_ptr<Window> window;
        bool validation_layers_enabled;
        std::string resources_dir;
        uint32_t max_frames_in_flight = 1;

        DeletionQueue deletion_queue;
        std::shared_ptr<VulkanContext> vulkan_context;

        std::shared_ptr<RaytracingRenderer> raytracing_renderer;
        std::shared_ptr<GuiRenderer> gui_renderer;
        std::shared_ptr<ComputeRenderer> glitch_renderer;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_RENDERINGMANAGER_HPP