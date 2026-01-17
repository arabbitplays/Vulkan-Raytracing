//
// Created by oschdi on 17.01.26.
//

#ifndef VULKAN_RAYTRACING_METALROUGHINSTANCE_HPP
#define VULKAN_RAYTRACING_METALROUGHINSTANCE_HPP
#include "MaterialInstance.hpp"

namespace RtEngine {
    class PhongInstance final : public MaterialInstance {
    public:
        PhongInstance() : MaterialInstance() {
            resources = std::make_shared<PhongResources>();
        }

        void *getResources(size_t *size) override;
        void loadResources(YAML::Node yaml_node) override;
        YAML::Node writeResourcesToYaml() override;

    protected:
        void initializeInstanceProperties() override;

        struct PhongResources {
            glm::vec3 diffuse;
            glm::vec3 specular;
            glm::vec3 ambient;
            glm::vec3 reflection;
            glm::vec3 transmission;
            float n;
            glm::vec4 eta; // only xyz for the eta of each rgb channel
        };

        glm::vec3 diffuse = glm::vec3(0.0f);
        glm::vec3 specular = glm::vec3(0.0f);
        glm::vec3 ambient = glm::vec3(0.0f);
        glm::vec3 reflection = glm::vec3(0.0f);
        glm::vec3 transmission = glm::vec3(0.0f);
        float n = 5.0f;
        glm::vec3 eta = glm::vec3(1.0f);

        std::shared_ptr<PhongResources> resources;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_METALROUGHINSTANCE_HPP