#ifndef VULKAN_RAYTRACING_VOLUMEASSET_HPP
#define VULKAN_RAYTRACING_VOLUMEASSET_HPP
#include <cstdint>
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "MeshAsset.hpp"

namespace RtEngine {
    struct VolumeBuffers {
        glm::uvec3 size;
        std::vector<glm::vec2> coefficients; // absorption scattering
    };

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
        float g;
        float majorant;
        std::shared_ptr<VolumeBuffers> volume_buffers;
        std::shared_ptr<MeshAsset> bounding_mesh;

        static std::shared_ptr<VolumeAsset> createHomogenous(const std::string &name, const float absorption, const float scattering, const float g, float majorant, const std::shared_ptr<MeshAsset> &bounding_mesh) {
            auto result = std::make_shared<VolumeAsset>();
            result->name = name;
            result->g = g;
            result->majorant = scattering + absorption + majorant;
            result->bounding_mesh = bounding_mesh;

            result->volume_buffers = std::make_shared<VolumeBuffers>(
                glm::uvec3(100),
                std::vector<glm::vec2>(1000000, glm::vec2(absorption, scattering))
            );

            return result;
        }
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_VOLUMEASSET_HPP