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
}
