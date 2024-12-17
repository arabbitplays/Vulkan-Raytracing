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
        AllocatedBuffer data_buffer;
    };

    PhongMaterial(VkDevice& device) : Material(device) {}

    void buildPipelines(VkDescriptorSetLayout sceneLayout) override;
    std::shared_ptr<MaterialInstance> writeMaterial(MaterialRessources ressources);
};



#endif //PHONGMATERIAL_HPP
