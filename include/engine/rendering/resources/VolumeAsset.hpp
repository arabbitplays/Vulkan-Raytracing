#ifndef VULKAN_RAYTRACING_VOLUMEASSET_HPP
#define VULKAN_RAYTRACING_VOLUMEASSET_HPP
#include <algorithm>
#include <cstdint>
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include "MeshAsset.hpp"
#include "Volume.hpp"

namespace RtEngine {
    struct VolumeData {
        float g;
        float max_scattering;
        float max_absorption;
        uint32_t volume_texture_idx;
        // Precomputed similarity-relation lookup (matches the shader-side
        // findBestIndex over the table keys); see VolumeManager::createVolumeData.
        float similarity_alpha;
        int32_t similarity_table_idx;
        uint32_t _pad0; // std430: keep the vec4 members 16-byte aligned
        uint32_t _pad1;
        glm::vec4 bounding_box_origin;
        glm::vec4 bounding_box_extent;
        glm::vec4 homogenized_scattering;
        glm::vec4 homogenized_absorption;
        // Inverse of the volume node's world transform, so shaders can map
        // world positions into the volume without hit-shader instance context.
        glm::mat4 world_to_object;
    };

    struct VolumeAsset
    {
        std::string name;
        uint32_t volume_id;
        std::shared_ptr<Volume> volume;
        // World transform of the node rendering this volume, stamped during
        // asset collection (SceneUtil::collectVolumeAssets). Assumes one
        // instance per volume asset; animated volumes need a re-collect
        // (VOLUME_UPDATE) to stay in sync.
        glm::mat4 world_transform = glm::mat4(1.0f);
        float g;
        glm::vec3 absorption_scale, scattering_scale;
        // Target optical depth across the longest world-space axis of the
        // volume, applied uniformly across scattering + absorption. 0 keeps
        // the density-normalized behavior (density_scale = 1/max_density).
        float optical_depth = 0.0f;
        std::shared_ptr<MeshAsset> bounding_mesh;

        // Per-voxel multiplier that turns raw density into sigma before the
        // per-channel scale is applied. Either 1/max_density (unscaled) or
        // optical_depth / (max_density * world_extent).
        float getDensityScale() const {
            if (volume->max_density <= 0.0f) {
                return 0.0f;
            }
            if (optical_depth <= 0.0f) {
                return 1.0f / volume->max_density;
            }
            const float extent = getMaxWorldExtent();
            if (extent <= 0.0f) {
                return 1.0f / volume->max_density;
            }
            return optical_depth / (volume->max_density * extent);
        }

        std::shared_ptr<std::vector<glm::vec4>> getScatteringCoefficients() const {
            auto coefficients = std::make_shared<std::vector<glm::vec4>>();
            coefficients->reserve(volume->size.x * volume->size.y * volume->size.z);
            const float density_scale = getDensityScale();
            for (uint32_t i = 0; i < volume->densities.size(); i++) {
                float density = volume->densities.at(i);
                coefficients->emplace_back(density * density_scale * scattering_scale.x,
                    density * density_scale * scattering_scale.y,
                    density * density_scale * scattering_scale.z,
                    0);
            }
            return coefficients;
        }

        std::shared_ptr<std::vector<glm::vec4>> getAbsorptionCoefficients() const {
            auto coefficients = std::make_shared<std::vector<glm::vec4>>();
            coefficients->reserve(volume->size.x * volume->size.y * volume->size.z);
            const float density_scale = getDensityScale();
            for (uint32_t i = 0; i < volume->densities.size(); i++) {
                float density = volume->densities.at(i);
                coefficients->emplace_back(density * density_scale * absorption_scale.x,
                    density * density_scale * absorption_scale.y,
                    density * density_scale * absorption_scale.z,
                    0);
            }
            return coefficients;
        }

        float getMaxScattering() const {
            const float peak = volume->max_density * getDensityScale();
            return peak * std::max(scattering_scale.x, std::max(scattering_scale.y, scattering_scale.z));
        }

        float getMaxAbsorption() const {
            const float peak = volume->max_density * getDensityScale();
            return peak * std::max(absorption_scale.x, std::max(absorption_scale.y, absorption_scale.z));
        }

        glm::vec3 getHomogenizedScattering(bool use_median = false) const {
            const float density = use_median ? volume->median_density : volume->avg_density;
            return density * getDensityScale() * scattering_scale;
        }

        glm::vec3 getHomogenizedAbsorption(bool use_median = false) const {
            const float density = use_median ? volume->median_density : volume->avg_density;
            return density * getDensityScale() * absorption_scale;
        }

    private:
        float getMaxWorldExtent() const {
            if (!bounding_mesh) return 0.0f;
            const auto aabb = bounding_mesh->calcAABB();
            const glm::mat3 rot(world_transform);
            const glm::vec3 world_extents(
                glm::length(rot * glm::vec3(aabb.extent.x, 0.0f, 0.0f)),
                glm::length(rot * glm::vec3(0.0f, aabb.extent.y, 0.0f)),
                glm::length(rot * glm::vec3(0.0f, 0.0f, aabb.extent.z))
            );
            return std::max({world_extents.x, world_extents.y, world_extents.z});
        }
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_VOLUMEASSET_HPP
