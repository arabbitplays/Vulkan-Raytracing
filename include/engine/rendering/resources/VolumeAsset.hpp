#ifndef VULKAN_RAYTRACING_VOLUMEASSET_HPP
#define VULKAN_RAYTRACING_VOLUMEASSET_HPP
#include <cstdint>
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "MeshAsset.hpp"
#include "Volume.hpp"

namespace RtEngine {
    struct VolumeData {
        float g;
        float majorant;
        uint32_t volume_texture_idx;
        float pad;
        glm::vec4 bounding_box_origin;
        glm::vec4 bounding_box_extent;
    };

    struct VolumeAsset {
        std::string name;
        uint32_t volume_id;
        std::shared_ptr<Volume> volume;
        float g;
        float absorption_scale, scattering_scale;
        std::shared_ptr<MeshAsset> bounding_mesh;

        std::vector<glm::vec2> getCoefficients() const {
            std::vector<glm::vec2> coefficients;
            coefficients.reserve(volume->size.x * volume->size.y * volume->size.z);
            for (uint32_t i = 0; i < volume->densities.size(); i++) {
                float density = volume->densities.at(i);
                coefficients.emplace_back(density / volume->max_density * absorption_scale, density / volume->max_density * scattering_scale);
            }
            return coefficients;
        }

        float getMajorant() {
            return absorption_scale + scattering_scale;
        }
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_VOLUMEASSET_HPP