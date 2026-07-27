#ifndef VULKAN_RAYTRACING_VOLUMEMANAGER_HPP
#define VULKAN_RAYTRACING_VOLUMEMANAGER_HPP
#include <memory>

#include "SimilarityTable.hpp"
#include "VolumeAsset.hpp"
#include "VulkanContext.hpp"

namespace RtEngine {
    class VolumeManager {
    public:
        VolumeManager() = default;

        explicit VolumeManager(const std::shared_ptr<VulkanContext> &vulkan_context) : vulkan_context(vulkan_context) {
            // Fallback bound to the unused slots of scattering/absorption samplers.
            float black[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            default_volume_texture = vulkan_context->resource_builder->createImage(
                (void *) &black, VkExtent3D{1, 1, 1}, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_TYPE_3D);
        }

        ~VolumeManager() {
        }

        void createVolumeResources(const std::vector<std::shared_ptr<VolumeAsset> > &volume_assets);

        void setSimilarityTable(const std::shared_ptr<SimilarityTable> &table) { similarity_table = table; }

        // Returns true if the flag changed (caller should trigger VOLUME_UPDATE
        // to rebuild the volume mapping buffer with the new avg-source values).
        bool setUseMedianDensity(bool use_median) {
            if (use_median_density == use_median) return false;
            use_median_density = use_median;
            return true;
        }
        bool getUseMedianDensity() const { return use_median_density; }

        VolumeData createVolumeData(const std::shared_ptr<VolumeAsset> &volume_asset, uint32_t texture_idx) const;

        void writeVolumeResources(VkSampler sampler) const;

        void reset();
        void destroy();

    private:
        AllocatedImage createVolumeTexture(glm::uvec3 vol_size, std::shared_ptr<std::vector<glm::vec4>> coefficients) const;

        std::shared_ptr<VulkanContext> vulkan_context;
        std::shared_ptr<SimilarityTable> similarity_table;
        AllocatedBuffer volume_mapping_buffer;
        std::vector<AllocatedImage> scattering_textures;
        std::vector<AllocatedImage> absorption_textures;

        AllocatedImage default_volume_texture;

        bool use_median_density = false;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_VOLUMEMANAGER_HPP
