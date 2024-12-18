//
// Created by oschdi on 12/17/24.
//

#ifndef PHONGMATERIAL_HPP
#define PHONGMATERIAL_HPP
#include <Material.hpp>
#include <glm/vec3.hpp>


class PhongMaterial : public Material {
public:
    struct MaterialConstants {
        glm::vec3 diffuse;
        glm::vec3 specular;
        glm::vec3 ambient;
        glm::vec3 properies; // n
    };

    struct MaterialRessources {
        std::shared_ptr<MaterialConstants> constants;
        // add images and samplers here
    };

    PhongMaterial(VkDevice& device, RessourceBuilder& ressource_builder_) : Material(device), ressource_builder(ressource_builder_) {}

    void buildPipelines(VkDescriptorSetLayout sceneLayout) override;
    void writeMaterial() override;
    std::shared_ptr<MaterialInstance> createInstance(glm::vec3 diffuse, glm::vec3 specular, glm::vec3 ambient, float n);

private:
    AllocatedBuffer createMaterialBuffer();

    std::vector<std::shared_ptr<MaterialInstance>> instances;
    std::vector<std::shared_ptr<MaterialConstants>> constants_buffer;
    AllocatedBuffer materialBuffer;

    RessourceBuilder ressource_builder;
};



#endif //PHONGMATERIAL_HPP
