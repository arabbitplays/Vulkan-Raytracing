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

        void createVolumeBuffer(const std::vector<std::shared_ptr<VolumeAsset>> &volume_assets);
        void writeVolumeBuffer() const;

        void destroy();

    private:
        std::shared_ptr<VulkanContext> vulkan_context;
        AllocatedBuffer volume_buffer;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_VOLUMEMANAGER_HPP