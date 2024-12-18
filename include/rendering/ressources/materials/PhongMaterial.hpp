//
// Created by oschdi on 12/17/24.
//

#ifndef PHONGMATERIAL_HPP
#define PHONGMATERIAL_HPP
#include <Material.hpp>


class PhongMaterial : public Material {
public:
    struct MaterialConstants {
        glm::vec4 albedo;
        glm::vec4 properies; // diffuse, specular, ambient
    };

    struct MaterialRessources {
        std::shared_ptr<MaterialConstants> constants;
        // add images and samplers here
    };

    PhongMaterial(VkDevice& device, RessourceBuilder& ressource_builder_) : Material(device), ressource_builder(ressource_builder_) {}

    void buildPipelines(VkDescriptorSetLayout sceneLayout) override;
    std::shared_ptr<MaterialInstance> writeMaterial(std::shared_ptr<MaterialRessources>& ressources);
    AllocatedBuffer createMaterialBuffer();

private:
    std::vector<std::shared_ptr<MaterialInstance>> instances;
    std::vector<std::shared_ptr<MaterialConstants>> constants;

    RessourceBuilder ressource_builder;
};



#endif //PHONGMATERIAL_HPP
