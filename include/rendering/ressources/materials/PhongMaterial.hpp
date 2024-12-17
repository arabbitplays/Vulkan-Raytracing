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

    PhongMaterial(VkDevice& device) : Material(device) {}

    void buildPipelines() override;
};



#endif //PHONGMATERIAL_HPP
