#ifndef VULKAN_RAYTRACING_METALROUGHINSTANCE_HPP
#define VULKAN_RAYTRACING_METALROUGHINSTANCE_HPP
#include <glm/gtc/epsilon.hpp>

#include "MaterialInstance.hpp"
#include "TextureRepository.hpp"

namespace RtEngine {
    class MetalRoughInstance final : public MaterialInstance {
    public:
        explicit MetalRoughInstance(const std::shared_ptr<TextureRepository<>>& tex_repo) : MaterialInstance(), tex_repo(tex_repo) {
            resources = std::make_shared<MetalRoughResources>();
        }

        void *getResources(size_t *size) override;
        void loadResources(YAML::Node yaml_node) override;
        YAML::Node writeResourcesToYaml() override;

        float getEmissionPower() override;

    protected:
        void initializeInstanceProperties() override;

        struct MetalRoughResources {
            glm::vec3 albedo;
            float padding;
            glm::vec4 properties; // metallic roughness ao eta
            glm::vec4 emission;
            glm::ivec4 tex_indices; // albedo, metal_rough_ao, normal
            bool operator==(const MetalRoughResources &other) const {
                return glm::all(glm::epsilonEqual(albedo, other.albedo, 0.0001f)) &&
                       glm::all(glm::epsilonEqual(properties, other.properties, 0.0001f)) &&
                       glm::all(glm::epsilonEqual(emission, other.emission, 0.0001f)) &&
                       glm::all(glm::equal(tex_indices, other.tex_indices));
            }
        };

        std::shared_ptr<TextureRepository<>> tex_repo;

        std::shared_ptr<Texture> albedo_tex;
        std::shared_ptr<Texture> metal_rough_ao_tex;
        std::shared_ptr<Texture> normal_tex;

        glm::vec3 albedo = glm::vec3(0.0f);
        float metallic = 0.5f, roughness = 0.5f, ao = 0.5f, eta = 1;
        glm::vec3 emission_color = glm::vec3(1.0);
        float emission_power = 0;

        std::shared_ptr<MetalRoughResources> resources;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_METALROUGHINSTANCE_HPP