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
            uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
            default_volume_texture = vulkan_context->resource_builder->createImage(
                (void *) &black, VkExtent3D{1, 1, 1}, VK_FORMAT_R32G32_SFLOAT, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_TYPE_3D);
        }

        ~VolumeManager() {
        }

        void createVolumeResources(const std::vector<std::shared_ptr<VolumeAsset> > &volume_assets);

        void setSimilarityTable(const std::shared_ptr<SimilarityTable> &table) { similarity_table = table; }

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
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_VOLUMEMANAGER_HPP
