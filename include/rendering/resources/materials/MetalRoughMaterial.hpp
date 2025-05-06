#ifndef METALROUGHMATERIAL_HPP
#define METALROUGHMATERIAL_HPP

#include <Material.hpp>
#include <glm/vec3.hpp>
#include <TextureRepository.hpp>

#define METAL_ROUGH_MATERIAL_NAME "metal_rough"

namespace RtEngine {
struct MetalRoughParameters {
    std::string albedo_tex_name, metal_rough_ao_tex_name, normal_tex_name;
    glm::vec3 albedo = glm::vec3(0.0f);
    float metallic, roughness, ao, eta = 1;
    glm::vec3 emission_color = glm::vec3(1.0);
    float emission_power = 0;
};

class MetalRoughMaterial : public Material {
public:
    struct MaterialResources {
        glm::vec3 albedo;
        float padding;
        glm::vec4 properties; // metallic roughness ao eta
        glm::vec4 emission;
        glm::ivec4 tex_indices; // albedo, metal_rough_ao, normal
        bool operator==(const MaterialResources& other) const {
            return glm::all(glm::epsilonEqual(albedo, other.albedo, 0.0001f))
                && glm::all(glm::epsilonEqual(properties, other.properties, 0.0001f))
                && glm::all(glm::epsilonEqual(emission, other.emission, 0.0001f))
                && glm::all(glm::equal(tex_indices, other.tex_indices));
        }
    };

    struct MaterialProperties
    {
        int32_t normal_mapping = 0, sample_lights = 0, sample_bsdf = 0, russian_roulette = 0;
    };

    MetalRoughMaterial(std::shared_ptr<VulkanContext> context, VkSampler sampler) : Material(METAL_ROUGH_MATERIAL_NAME, context), sampler(sampler) {}

    void buildPipelines(VkDescriptorSetLayout sceneLayout) override;
    void writeMaterial() override;
    glm::vec4 getEmissionForInstance(uint32_t material_instance_id) override;
    std::vector<std::shared_ptr<MaterialResources>> getResources();
    std::vector<std::shared_ptr<Texture>> getTextures() override;

    std::shared_ptr<MaterialInstance> createInstance(MetalRoughParameters parameters, bool unique = false);
    void reset() override;

protected:
    void initProperties() override;
    std::shared_ptr<MaterialResources> createMaterialResources(const MetalRoughParameters& parameters);
    std::shared_ptr<PropertiesSection> initializeInstanceProperties(const std::shared_ptr<MaterialResources>& resources);
private:
    AllocatedBuffer createMaterialBuffer();


    std::vector<std::shared_ptr<MaterialResources>> resources_buffer;
    std::vector<std::shared_ptr<Texture>> albedo_textures, metal_rough_ao_textures, normal_textures;

    MaterialProperties material_properties;
    VkSampler sampler;
};



}
#endif //METALROUGHMATERIAL_HPP
