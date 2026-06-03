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

    struct VolumeAsset
    {
        std::string name;
        uint32_t volume_id;
        std::shared_ptr<Volume> volume;
        float g;
        glm::vec3 absorption_scale, scattering_scale;
        float bonus_majorant = 0;
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

        float getMajorant() const {
            glm::vec3 extinction = scattering_scale + absorption_scale;
            return std::max(extinction.x, std::max(extinction.y, extinction.z)) + bonus_majorant;
            // because densities get scaled down this is not max_density * (scattering_scale + absorption_scale)
        }
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_VOLUMEASSET_HPP
