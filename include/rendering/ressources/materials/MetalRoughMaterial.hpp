//
// Created by oschdi on 12/30/24.
//

#ifndef METALROUGHMATERIAL_HPP
#define METALROUGHMATERIAL_HPP

#include <Material.hpp>
#include <glm/vec3.hpp>

struct MetalRoughParameters {
    std::shared_ptr<Texture> albedo_tex, metal_rough_ao_tex, normal_tex;
    glm::vec3 albedo = glm::vec3(0.0f);
    float metallic, roughness, ao;
    glm::vec3 emission_color = glm::vec3(1.0);
    float emission_power = 0;
};

class MetalRoughMaterial : public Material {
public:
    struct MaterialConstants {
        glm::vec4 albedo;
        glm::vec4 properties; // metallic roughness ao
        glm::vec4 emission;
    };

    struct MaterialResources
    {
        std::shared_ptr<MaterialConstants> constants;
        Texture albedo_tex;
        Texture metal_rough_ao_tex;
        Texture normal_tex;
    };

    MetalRoughMaterial(std::shared_ptr<VulkanContext> context, VkSampler sampler) : Material(context), sampler(sampler) {}

    void buildPipelines(VkDescriptorSetLayout sceneLayout) override;
    void writeMaterial() override;
    glm::vec4 getEmissionForInstance(uint32_t material_instance_id) override;
    std::shared_ptr<MaterialResources> getResourcesForInstance(uint32_t material_instance_id);
    std::shared_ptr<MaterialInstance> createInstance(MetalRoughParameters parameters);
    void reset() override;
private:
    AllocatedBuffer createMaterialBuffer();

    std::shared_ptr<Texture> default_tex, default_normal_tex;

    std::vector<std::shared_ptr<MaterialInstance>> instances;
    std::vector<std::shared_ptr<MaterialResources>> resources_buffer;
    VkSampler sampler;
    AllocatedBuffer materialBuffer; // maps an instance to its respective material via a common index into the constants and texture buffers
};



#endif //METALROUGHMATERIAL_HPP
