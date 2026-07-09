#include "VolumeManager.hpp"

#include "AccelerationStructure.hpp"
#include "AccelerationStructure.hpp"
#include "AccelerationStructure.hpp"
#include "AccelerationStructure.hpp"
#include "AccelerationStructure.hpp"
#include "AccelerationStructure.hpp"
#include "AccelerationStructure.hpp"
#include "AccelerationStructure.hpp"
#include "AccelerationStructure.hpp"
#include "AccelerationStructure.hpp"
#include "AccelerationStructure.hpp"
#include "AccelerationStructure.hpp"
#include "AccelerationStructure.hpp"
#include "AccelerationStructure.hpp"
#include "AccelerationStructure.hpp"
#include "AccelerationStructure.hpp"
#include "AccelerationStructure.hpp"
#include "AccelerationStructure.hpp"
#include "QuickTimer.hpp"
#include "spdlog/spdlog.h"

namespace RtEngine {
    void VolumeManager::createVolumeResources(const std::vector<std::shared_ptr<VolumeAsset> > &volume_assets) {
        QuickTimer timer{"Volumes", true};

        reset();

        std::vector<VolumeData> volume_datas;
        uint volume_id = 0;
        for (auto &volume_asset: volume_assets) {
            volume_asset->volume_id = volume_id++;

            const uint32_t texture_idx = scattering_textures.size();
            AllocatedImage scattering_texture = createVolumeTexture(volume_asset->volume->size, volume_asset->getScatteringCoefficients());
            scattering_textures.push_back(scattering_texture);
            AllocatedImage absorption_texture = createVolumeTexture(volume_asset->volume->size, volume_asset->getAbsorptionCoefficients());
            absorption_textures.push_back(absorption_texture);

            VolumeData data = createVolumeData(volume_asset, texture_idx);
            volume_datas.push_back(data);
        }

        if (volume_datas.empty()) {
            volume_datas.push_back(VolumeData{});
        }

        volume_mapping_buffer = vulkan_context->resource_builder->stageMemoryToNewBuffer(
            volume_datas.data(), volume_datas.size() * sizeof(VolumeData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    }

    VolumeData VolumeManager::createVolumeData(const std::shared_ptr<VolumeAsset> &volume_asset, uint32_t texture_idx) const {
        auto [origin, extent] = volume_asset->bounding_mesh->calcAABB();

        if (!similarity_table || similarity_table->g_keys.empty()) {
            throw std::runtime_error("Similarity table not loaded before creating volume resources");
        }
        const int similarity_idx = similarity_table->findBestIndex(volume_asset->g);
        const float similarity_alpha = similarity_table->alphas.at(similarity_idx);
        spdlog::info("Volume '{}': g={} -> similarity table {} (g_key={}, alpha={})",
                     volume_asset->name, volume_asset->g, similarity_idx,
                     similarity_table->g_keys.at(similarity_idx), similarity_alpha);

        const VolumeData volume_data{
            .g = volume_asset->g,
            .max_scattering = volume_asset->getMaxScattering(),
            .max_absorption = volume_asset->getMaxAbsorption(),
            .volume_texture_idx = texture_idx,
            .similarity_alpha = similarity_alpha,
            .similarity_table_idx = similarity_idx,
            .bounding_box_origin = glm::vec4(origin, 0),
            .bounding_box_extent = glm::vec4(extent, 0),
            .avg_scattering = glm::vec4(volume_asset->getAvgScattering(), 0),
            .avg_absorption = glm::vec4(volume_asset->getAvgAbsorption(), 0),
        };
        return volume_data;
    }

    AllocatedImage VolumeManager::createVolumeTexture(glm::uvec3 vol_size, std::shared_ptr<std::vector<glm::vec4>> coefficients) const {
        VkExtent3D extent = {vol_size.x, vol_size.y, vol_size.z};
        return vulkan_context->resource_builder->createImage(coefficients->data(), extent, VK_FORMAT_R32G32B32A32_SFLOAT,
                                                             VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT,
                                                             VK_IMAGE_ASPECT_COLOR_BIT,
                                                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                             VK_IMAGE_TYPE_3D);
    }

    void VolumeManager::writeVolumeResources(VkSampler sampler) const {
        vulkan_context->descriptor_allocator->writeBuffer(8, volume_mapping_buffer.handle, 0,
                                                          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        assert(scattering_textures.size() == absorption_textures.size());
        std::vector<VkImageView> scattering_views = {};
        std::vector<VkImageView> absorption_views = {};
        scattering_views.reserve(16);
        absorption_views.reserve(16);
        for (uint32_t i = 0; i < scattering_textures.size(); i++) {
            scattering_views.push_back(scattering_textures[i].imageView);
            absorption_views.push_back(absorption_textures[i].imageView);
        }

        for (uint32_t i = scattering_views.size(); i < 16; i++) {
            scattering_views.push_back(default_volume_texture.imageView);
            absorption_views.push_back(default_volume_texture.imageView);
        }
        vulkan_context->descriptor_allocator->writeImages(11, scattering_views, sampler,
                                                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        vulkan_context->descriptor_allocator->writeImages(12, absorption_views, sampler,
                                                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }

    void VolumeManager::reset() {
        if (volume_mapping_buffer.handle != VK_NULL_HANDLE)
            vulkan_context->resource_builder->destroyBuffer(volume_mapping_buffer);

        for (auto &texture: scattering_textures) {
            vulkan_context->resource_builder->destroyImage(texture);
        }
        scattering_textures.clear();

        for (auto &texture: absorption_textures) {
            vulkan_context->resource_builder->destroyImage(texture);
        }
        absorption_textures.clear();
    }

    void VolumeManager::destroy() {
        reset();
        vulkan_context->resource_builder->destroyImage(default_volume_texture);
    }
} // RtEngine
