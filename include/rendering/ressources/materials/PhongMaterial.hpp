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
    void writeMaterial() override;
    std::shared_ptr<MaterialInstance> addInstance(std::shared_ptr<MaterialRessources>& ressources);

private:
    AllocatedBuffer createMaterialBuffer();

    std::vector<std::shared_ptr<MaterialInstance>> instances;
    std::vector<std::shared_ptr<MaterialConstants>> constants;
    AllocatedBuffer materialBuffer;

    RessourceBuilder ressource_builder;
};



#endif //PHONGMATERIAL_HPP
