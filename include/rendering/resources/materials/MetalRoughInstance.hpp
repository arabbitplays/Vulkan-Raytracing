//
// Created by oschdi on 09.09.25.
//

#ifndef VULKAN_RAYTRACING_METALROUGHINSTANCE_HPP
#define VULKAN_RAYTRACING_METALROUGHINSTANCE_HPP
#include <memory>
#include <string>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "MaterialInstance.hpp"
#include "Texture.hpp"
#include "TextureRepository.hpp"

namespace RtEngine {
    class MetalRoughInstance : public MaterialInstance {
    public:
        struct Parameters {
            std::string albedo_tex_name, metal_rough_ao_tex_name, normal_tex_name;
            glm::vec3 albedo = glm::vec3(0.0f);
            float metallic, roughness, ao, eta = 1;
            glm::vec3 emission_color = glm::vec3(1.0);
            float emission_power = 0;
        };

        MetalRoughInstance() = default;
        MetalRoughInstance(const Parameters& parameters, const std::shared_ptr<TextureRepository> &texture_repository);
        void attachTo(Material &m) override;

        glm::vec3 albedo;
        float metallic;
        float roughness;
        float ao;
        float eta;

        glm::vec3 emission_color;
        float emission_power;

        std::shared_ptr<Texture> albedo_tex, metal_rough_ao_tex, normal_tex;
    };
}

#endif //VULKAN_RAYTRACING_METALROUGHINSTANCE_HPP