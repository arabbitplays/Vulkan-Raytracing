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

        resources->tex_indices =
                glm::vec4{tex_repo->getTextureIndex(albedo_tex->name),
                          tex_repo->getTextureIndex(metal_rough_ao_tex->name),
                          tex_repo->getTextureIndex(normal_tex->name), 0};

        *size = sizeof(MetalRoughResources);
        return resources.get();
    }

    void MetalRoughInstance::loadResources(YAML::Node yaml_node) {
        name = yaml_node["name"].as<std::string>();
        if (yaml_node["albedo_tex"]) {
            albedo_tex = tex_repo->addTexture(yaml_node["albedo_tex"].as<std::string>(), PARAMETER);
            albedo = glm::vec3(0.0f);
        } else {
            albedo_tex = tex_repo->getDefaultTex(PARAMETER);
            if (yaml_node["albedo"])
                albedo = yaml_node["albedo"].as<glm::vec3>();
        }

        if (yaml_node["metal_rough_ao_tex"]) {
            metal_rough_ao_tex = tex_repo->addTexture(yaml_node["metal_rough_ao_tex"].as<std::string>(), PARAMETER);
            metallic = 0.0f;
            roughness = 0.0f;
            ao = 0.0f;
        } else {
            metal_rough_ao_tex = tex_repo->getDefaultTex(PARAMETER);
            if (yaml_node["metallic"])
                metallic = yaml_node["metallic"].as<float>();
            if (yaml_node["roughness"])
                roughness = yaml_node["roughness"].as<float>();
            if (yaml_node["ao"])
                ao = yaml_node["ao"].as<float>();
        }

        if (yaml_node["eta"])
            eta = yaml_node["eta"].as<float>();

        if (yaml_node["normal_tex"])
            normal_tex = tex_repo->addTexture(yaml_node["normal_tex"].as<std::string>(), NORMAL);
        else
            normal_tex = tex_repo->getDefaultTex(NORMAL);

        if (yaml_node["emission_power"]) {
            emission_color = yaml_node["emission_color"].as<glm::vec3>();
            emission_power = yaml_node["emission_power"].as<float>();
        }
    }

    YAML::Node MetalRoughInstance::writeResourcesToYaml() {
        std::string default_tex_name = tex_repo->getDefaultTex(PARAMETER)->name;
        std::string default_normal_tex_name = tex_repo->getDefaultTex(NORMAL)->name;

        YAML::Node out(YAML::NodeType::Map);
        out["name"] = name;

        if (albedo_tex->name == default_tex_name) {
            if (albedo != glm::vec3(0.0f)) {
                out["albedo"] = YAML::convert<glm::vec3>::encode(albedo);
            }
        } else {
            out["albedo_tex"] = albedo_tex->path;
        }

        if (metal_rough_ao_tex->name == default_tex_name) {
            if (albedo != glm::vec3(0.0f)) {
                out["metallic"] = metallic;
                out["roughness"] = roughness;
                out["ao"] = ao;
            }
        } else {
            out["metal_rough_ao_tex"] = metal_rough_ao_tex->path;
        }

        out["eta"] = eta;

        if (normal_tex->name != default_normal_tex_name) {
            out["normal_tex"] = normal_tex->path;
        }

        if (emission_power != 0.0f) {
            out["emission_color"] = YAML::convert<glm::vec3>::encode(emission_color);
            out["emission_power"] = emission_power;
        }

        return out;
    }

    float MetalRoughInstance::getEmissionPower() {
        return emission_power;
    }
} // RtEngine