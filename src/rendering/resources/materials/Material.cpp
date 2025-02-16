//
// Created by oschdi on 12/17/24.
//

#include "Material.hpp"

std::vector<std::shared_ptr<MaterialInstance>> Material::getInstances()
{
    return instances;
}

void Material::clearRessources() {
    resetQueue.flush();
    mainDeletionQueue.flush();
}

void Material::reset() {
    resetQueue.flush();
}
