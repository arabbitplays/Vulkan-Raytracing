#include "MetalRoughInstance.hpp"

namespace RtEngine {
    void MetalRoughInstance::initializeInstanceProperties() {
        properties = std::make_shared<PropertiesSection>("Metal Rough Material");
        properties->addVector("Albedo", &albedo);
        properties->addFloat("Metal", &metallic, ALL_PROPERTY_FLAGS, 0, 1);
        properties->addFloat("Roughness", &roughness, ALL_PROPERTY_FLAGS, 0, 1);
        properties->addFloat("Eta", &eta);
        properties->addVector("Emission Color", &emission_color);
        properties->addFloat("Emission Power", &emission_power);
    }

    void *MetalRoughInstance::getResources(size_t *size) {
        resources->albedo = glm::vec4(albedo, 0.0f);
        resources->properties = glm::vec4(metallic, roughness, ao, eta);
        resources->emission = glm::vec4(emission_color, emission_power);

        // TODO bring them back
        /*auto add_texture_if_needed = [&](const std::string &texture_name,
                                         std::vector<std::shared_ptr<Texture>> &textures,
                                         bool is_normal_map = false) -> uint32_t {
            for (uint32_t i = 0; i < textures.size(); i++) {
                if (textures[i]->name == texture_name) {
                    return i;
                }
            }

            textures.push_back(runtime_context->texture_repository->getTexture(texture_name));
            return textures.size() - 1;
        };

        resources->tex_indices =
                glm::vec4{add_texture_if_needed(albedo_tex_name, albedo_textures),
                          add_texture_if_needed(metal_rough_ao_tex_name, metal_rough_ao_textures),
                          add_texture_if_needed(normal_tex_name, normal_textures, true), 0};*/

        *size = sizeof(MetalRoughResources);
        return resources.get();
    }

    void MetalRoughInstance::loadResources(YAML::Node yaml_node) {
        if (yaml_node["albedo"])
            albedo = yaml_node["albedo"].as<glm::vec3>();
        if (yaml_node["albedo_tex"])
            albedo_tex_name = yaml_node["albedo_tex"].as<std::string>();

        if (yaml_node["metallic"])
            metallic = yaml_node["metallic"].as<float>();
        if (yaml_node["roughness"])
            roughness = yaml_node["roughness"].as<float>();
        if (yaml_node["ao"])
            ao = yaml_node["ao"].as<float>();
        if (yaml_node["metal_rough_ao_tex"])
            metal_rough_ao_tex_name = yaml_node["metal_rough_ao_tex"].as<std::string>();

        if (yaml_node["eta"])
            eta = yaml_node["eta"].as<float>();

        if (yaml_node["normal_tex"])
            normal_tex_name = yaml_node["normal_tex"].as<std::string>();

        if (yaml_node["emission_power"]) {
            emission_color = yaml_node["emission_color"].as<glm::vec3>();
            emission_power = yaml_node["emission_power"].as<float>();
        }
    }

    float MetalRoughInstance::getEmissionPower() {
        return emission_power;
    }
} // RtEngine