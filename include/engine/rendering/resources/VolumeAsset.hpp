#ifndef VULKAN_RAYTRACING_VOLUMEASSET_HPP
#define VULKAN_RAYTRACING_VOLUMEASSET_HPP
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
        std::shared_ptr<MeshAsset> bounding_mesh;

        std::shared_ptr<std::vector<glm::vec4>> getScatteringCoefficients() const {
            auto coefficients = std::make_shared<std::vector<glm::vec4>>();
            coefficients->reserve(volume->size.x * volume->size.y * volume->size.z);
            for (uint32_t i = 0; i < volume->densities.size(); i++) {
                float density = volume->densities.at(i);
                coefficients->emplace_back(density / volume->max_density * scattering_scale.x,
                    density / volume->max_density * scattering_scale.y,
                    density / volume->max_density * scattering_scale.z,
                    0);
            }
            return coefficients;
        }

        std::shared_ptr<std::vector<glm::vec4>> getAbsorptionCoefficients() const {
            auto coefficients = std::make_shared<std::vector<glm::vec4>>();
            coefficients->reserve(volume->size.x * volume->size.y * volume->size.z);
            for (uint32_t i = 0; i < volume->densities.size(); i++) {
                float density = volume->densities.at(i);
                coefficients->emplace_back(density / volume->max_density * absorption_scale.x,
                    density / volume->max_density * absorption_scale.y,
                    density / volume->max_density * absorption_scale.z,
                    0);
            }
            return coefficients;
        }

        float getMaxScattering() const {
            // because density gets scaled down
            return std::max(scattering_scale.x, std::max(scattering_scale.y, scattering_scale.z));
        }

        float getMaxAbsorption() const {
            // because density gets scaled down
            return std::max(absorption_scale.x, std::max(absorption_scale.y, absorption_scale.z));
        }

        glm::vec3 getHomogenizedScattering(bool use_median = false) const {
            const float density = use_median ? volume->median_density : volume->avg_density;
            return density / volume->max_density * scattering_scale;
        }

        glm::vec3 getHomogenizedAbsorption(bool use_median = false) const {
            const float density = use_median ? volume->median_density : volume->avg_density;
            return density / volume->max_density * absorption_scale;
        }
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_VOLUMEASSET_HPP
