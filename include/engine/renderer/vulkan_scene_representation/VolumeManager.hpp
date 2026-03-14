#ifndef VULKAN_RAYTRACING_VOLUMEMANAGER_HPP
#define VULKAN_RAYTRACING_VOLUMEMANAGER_HPP
#include <memory>

#include "VolumeAsset.hpp"
#include "VulkanContext.hpp"

namespace RtEngine {
    class VolumeManager {
    public:
        VolumeManager() = default;
        explicit VolumeManager(const std::shared_ptr<VulkanContext> &vulkan_context) :
            vulkan_context(vulkan_context) {}

        void createVolumeResources(const std::vector<std::shared_ptr<VolumeAsset>> &volume_assets);

        static VolumeData createVolumeData(const std::shared_ptr<VolumeAsset> &volume_asset, uint32_t texture_idx);

        AllocatedImage createVolumeTexture(const std::shared_ptr<VolumeBuffers> &volume_buffers) const;

        void writeVolumeResources(VkSampler sampler) const;

        void destroy();

    private:
        std::shared_ptr<VulkanContext> vulkan_context;
        AllocatedBuffer volume_mapping_buffer;
        std::vector<AllocatedImage> volume_textures;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_VOLUMEMANAGER_HPP