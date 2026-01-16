//
// Created by oschdi on 17.09.25.
//

#ifndef VULKAN_RAYTRACING_PHONGINSTANCE_HPP
#define VULKAN_RAYTRACING_PHONGINSTANCE_HPP
#include "../../engine/core/materials/MaterialInstance.hpp"

namespace RtEngine {
    class PhongMaterial;

    class PhongInstance : public MaterialInstance {
        friend PhongMaterial;

    public:
        struct Parameters {
            glm::vec3 diffuse = glm::vec3(1.0);
            glm::vec3 specular = glm::vec3(1.0);
            glm::vec3 ambient = glm::vec3(0.0);
            glm::vec3 reflection = glm::vec3(0);
            glm::vec3 transmission = glm::vec3(0);
            float n = 1;
            glm::vec3 eta = glm::vec3(1);
        };

        PhongInstance() = default;
        PhongInstance(const Parameters& parameters);
        void attachTo(Material &m) override;

    private:
        void initializeProperties() override;

        glm::vec3 diffuse;
        glm::vec3 specular;
        glm::vec3 ambient;

        glm::vec3 reflection;
        glm::vec3 transmission;
        float n;
        glm::vec3 eta;

    };
} // RtEngine

#endif //VULKAN_RAYTRACING_PHONGINSTANCE_HPP