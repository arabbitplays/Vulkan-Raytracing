#ifndef VULKAN_RAYTRACING_VOLUMEREPOSITORY_HPP
#define VULKAN_RAYTRACING_VOLUMEREPOSITORY_HPP
#include <memory>

#include "DeletionQueue.hpp"
#include "VolumeAsset.hpp"
#include "VolumeBuilder.hpp"
#include "VulkanContext.hpp"

namespace RtEngine {
    namespace fs = std::filesystem;

    class VolumeRepository {
    public:
        VolumeRepository() = default;
        VolumeRepository(const std::shared_ptr<VulkanContext> &context, const std::string &resource_dir);

        std::shared_ptr<Volume> getOrCreateVolume(const std::string &path);
        std::shared_ptr<VolumeAsset> createHeterogenousVolumeAsset(const fs::path &path, float absorption, float scattering, float g, const std::shared_ptr<MeshAsset> &mesh_asset);
        static std::shared_ptr<VolumeAsset> createHomogenousVolumeAsset(float absorption, float scattering, float g, float majorant,
                                                           const std::shared_ptr<MeshAsset> &mesh_asset);
        void destroy();

    private:
        DeletionQueue deletion_queue;

        std::shared_ptr<VolumeBuilder> volume_builder;
        std::unordered_map<std::string, std::shared_ptr<Volume>> volume_path_cache;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_VOLUMEREPOSITORY_HPP