#ifndef VULKAN_RAYTRACING_VOLUMEREPOSITORY_HPP
#define VULKAN_RAYTRACING_VOLUMEREPOSITORY_HPP
#include <memory>

#include "DeletionQueue.hpp"
#include "VolumeAsset.hpp"
#include "VulkanContext.hpp"

namespace RtEngine {
    class VolumeRepository {
    public:
        VolumeRepository() = default;
        VolumeRepository(const std::shared_ptr<VulkanContext> &context, const std::string &resource_dir);

        std::shared_ptr<VolumeAsset> getVolume(const std::string &name);
        std::string addVolume(std::string path);
        void destroy();

    private:
        DeletionQueue deletion_queue;

        std::unordered_map<std::string, std::shared_ptr<VolumeAsset>> mesh_name_cache, mesh_path_cache;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_VOLUMEREPOSITORY_HPP