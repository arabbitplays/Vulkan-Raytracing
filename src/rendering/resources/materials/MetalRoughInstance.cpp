#include "MetalRoughInstance.hpp"

#include "MetalRoughMaterial.hpp"

namespace RtEngine {
    MetalRoughInstance::MetalRoughInstance(const Parameters& parameters, const std::shared_ptr<TextureRepository> &texture_repository) {
        albedo = parameters.albedo;
        metallic = parameters.metallic;
        roughness = parameters.roughness;
        ao = parameters.ao;
        eta = parameters.eta;

        emission_color = parameters.emission_color;
        emission_power = parameters.emission_power;

        albedo_tex = texture_repository->getTexture(parameters.albedo_tex_name);
        metal_rough_ao_tex = texture_repository->getTexture(parameters.metal_rough_ao_tex_name);
        normal_tex = texture_repository->getTexture(parameters.normal_tex_name);
    }

    void MetalRoughInstance::attachTo(Material &m) {
        if (auto* mat = dynamic_cast<MetalRoughMaterial*>(&m)) {
            mat->addInstanceToResources(*this);
        } else {
            throw std::runtime_error("Wrong material-instance pair");
        }
    }

    void MetalRoughInstance::initializeProperties() {
        properties = std::make_shared<PropertiesSection>("Metal Rough Material");
        properties->addVector("Albedo", &albedo);
        properties->addFloat("Metal", &metallic, ALL_PROPERTY_FLAGS, 0, 1);
        properties->addFloat("Roughness", &roughness, ALL_PROPERTY_FLAGS, 0, 1);
        properties->addFloat("Eta", &eta);
        properties->addVector("Emission Color", &emission_color);
        properties->addFloat("Emission Power", &emission_power);
    }
}
