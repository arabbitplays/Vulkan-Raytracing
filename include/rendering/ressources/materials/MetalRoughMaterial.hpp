//
// Created by oschdi on 12/30/24.
//

#ifndef METALROUGHMATERIAL_HPP
#define METALROUGHMATERIAL_HPP

#include <Material.hpp>
#include <glm/vec3.hpp>

class MetalRoughMaterial : public Material {
public:
    struct MaterialConstants {
        glm::vec3 albedo;
        glm::vec3 properties; // metallic roughness ao
        glm::vec3 eta;
        glm::vec3 padding;
    };

    struct MaterialRessources {
        std::shared_ptr<MaterialConstants> constants;
        // add images and samplers here
    };

    MetalRoughMaterial(std::shared_ptr<VulkanContext> context) : Material(context) {}

    void buildPipelines(VkDescriptorSetLayout sceneLayout) override;
    void writeMaterial() override;
    std::shared_ptr<MaterialInstance> createInstance(glm::vec3 albedo, float metallic, float roughness, float ao, glm::vec3 eta = glm::vec3(0.0));
    void reset() override;
private:
    AllocatedBuffer createMaterialBuffer();

    std::vector<std::shared_ptr<MaterialInstance>> instances;
    std::vector<std::shared_ptr<MaterialConstants>> constants_buffer;
    AllocatedBuffer materialBuffer;
};



#endif //METALROUGHMATERIAL_HPP
