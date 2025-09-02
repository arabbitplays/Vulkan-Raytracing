//
// Created by oschdi on 02.09.25.
//

#include "MaterialRepository.hpp"

#include "MetalRoughMaterial.hpp"
#include "PhongMaterial.hpp"

namespace RtEngine {
    void MaterialRepository::addMaterial(const std::shared_ptr<Material> &material) {
        if (materials.contains(material->name)) {
            spdlog::warn("Material '" + material->name + "' overwritten in repository");
        }
        materials[material->name] = material;
    }

    std::shared_ptr<Material> MaterialRepository::getMaterial(const std::string &name) {
        if (materials.contains(name))
            return materials[name];
        else
            return nullptr;
    }

    void MaterialRepository::resetMaterials() const {
        for (auto &material: materials) {
            material.second->reset();
        }
    }

    void MaterialRepository::destroyMaterials() const {
        for (auto &material: materials) {
            material.second->destroyResources();
        }
    }

} // RtEngine