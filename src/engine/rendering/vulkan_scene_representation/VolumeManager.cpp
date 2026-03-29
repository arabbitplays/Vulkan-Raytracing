#include "VolumeManager.hpp"

#include "QuickTimer.hpp"

namespace RtEngine {
    void VolumeManager::createVolumeResources(const std::vector<std::shared_ptr<VolumeAsset>> &volume_assets) {
        assert(!volume_assets.empty());

        QuickTimer timer{"Volumes", true};

        destroy();

        std::vector<VolumeData> volume_datas;
        uint volume_id = 0;
        for (auto &volume_asset: volume_assets) {
            volume_asset->volume_id = volume_id++;

            const uint32_t texture_idx = volume_textures.size();
            auto coefficients = volume_asset->getCoefficients();
            AllocatedImage texture = createVolumeTexture(volume_asset->volume->size, coefficients);
            volume_textures.push_back(texture);

            VolumeData data = createVolumeData(volume_asset, texture_idx);
            volume_datas.push_back(data);
        }

        volume_mapping_buffer = vulkan_context->resource_builder->stageMemoryToNewBuffer(
                volume_datas.data(), volume_assets.size() * sizeof(VolumeData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    }

    VolumeData VolumeManager::createVolumeData(const std::shared_ptr<VolumeAsset> &volume_asset, uint32_t texture_idx) {
        auto [origin, extent] = volume_asset->bounding_mesh->calcAABB();
        const VolumeData volume_data {
            .g = volume_asset->g,
            .majorant = volume_asset->volume->max_density,
            .volume_texture_idx = texture_idx,
            .bounding_box_origin = glm::vec4(origin, 0),
            .bounding_box_extent = glm::vec4(extent, 0),
        };
        return volume_data;
    }

    AllocatedImage VolumeManager::createVolumeTexture(glm::uvec3 vol_size, std::vector<glm::vec2>& coefficients) const {
        VkExtent3D extent = {vol_size.x, vol_size.y, vol_size.z };
        return vulkan_context->resource_builder->createImage(coefficients.data(), extent, VK_FORMAT_R32G32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL);
    }

    void VolumeManager::writeVolumeResources(VkSampler sampler) const {
        vulkan_context->descriptor_allocator->writeBuffer(8, volume_mapping_buffer.handle, 0,
                                                          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        std::vector<VkImageView> image_views = {};
        image_views.reserve(16);
        for (auto &texture: volume_textures) {
            image_views.push_back(texture.imageView);
        }

        for (uint32_t i = volume_textures.size(); i < 16; i++) {
            image_views.push_back(image_views.at(0));
        }
        vulkan_context->descriptor_allocator->writeImages(11, image_views, sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }

    void VolumeManager::destroy() {
        if (volume_mapping_buffer.handle != VK_NULL_HANDLE)
            vulkan_context->resource_builder->destroyBuffer(volume_mapping_buffer);
        for (auto& texture: volume_textures) {
            vulkan_context->resource_builder->destroyImage(texture);
        }
        volume_textures.clear();
    }
} // RtEngine