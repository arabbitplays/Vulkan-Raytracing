

#include "PhongInstance.hpp"

namespace RtEngine {
    void PhongInstance::initializeInstanceProperties() {
        properties = std::make_shared<PropertiesSection>("Phong Material");
    }

    void *PhongInstance::getResources(size_t *size) {
        resources->diffuse = diffuse;
        resources->specular = specular;
        resources->ambient = ambient;
        resources->reflection = reflection;
        resources->transmission = transmission;
        resources->n = n;
        resources->eta = glm::vec4(eta, 0.0f);

        *size = sizeof(PhongResources);
        return resources.get();
    }

    void PhongInstance::loadResources(YAML::Node yaml_node) {
        if (yaml_node["diffuse"])
            diffuse = yaml_node["diffuse"].as<glm::vec3>();
        if (yaml_node["specular"])
            specular = yaml_node["specular"].as<glm::vec3>();
        if (yaml_node["ambient"])
            ambient = yaml_node["ambient"].as<glm::vec3>();
        if (yaml_node["reflection"])
            reflection = yaml_node["reflection"].as<glm::vec3>();
        if (yaml_node["transmission"])
            transmission = yaml_node["transmission"].as<glm::vec3>();

        if (yaml_node["n"])
            n = yaml_node["n"].as<float>();
        if (yaml_node["eta"])
            eta = yaml_node["eta"].as<glm::vec3>();
    }

} // RtEngine