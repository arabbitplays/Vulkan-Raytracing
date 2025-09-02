//
// Created by oschdi on 02.09.25.
//

#ifndef VULKAN_RAYTRACING_DEFAULTRESOURCES_HPP
#define VULKAN_RAYTRACING_DEFAULTRESOURCES_HPP
#include <memory>

#include "MaterialRepository.hpp"
#include "VulkanContext.hpp"

namespace RtEngine {
    class DefaultResources {
    public:
        DefaultResources(
            const std::shared_ptr<VulkanContext>& vulkan_context,
            const std::shared_ptr<TextureRepository>& texture_repository,
            const std::shared_ptr<MaterialRepository>& material_repository);
        void destroy();

        VkSampler getLinearSampler() const;
    private:
        void createDefaultTextures();
        void createDefaultSamplers();
        void createDefaultMaterials(const VkPhysicalDeviceRayTracingPipelinePropertiesKHR &raytracingProperties);

        std::shared_ptr<VulkanContext> vulkan_context;
        std::shared_ptr<TextureRepository> texture_repository;
        std::shared_ptr<MaterialRepository> material_repository;
        DeletionQueue deletion_queue;

        AllocatedImage whiteImage;
        AllocatedImage greyImage;
        AllocatedImage blackImage;
        AllocatedImage errorCheckerboardImage;

        VkSampler defaultSamplerLinear;
        VkSampler defaultSamplerNearest;
        VkSampler defaultSamplerAnisotropic;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_DEFAULTRESOURCES_HPP