//
// Created by oschdi on 12/17/24.
//

#include "Material.hpp"

std::vector<std::shared_ptr<MaterialInstance>> Material::getInstances()
{
    return instances;
}

std::shared_ptr<Properties> Material::getProperties()
{
    if (properties == nullptr)
    {
        initProperties();
    }

    assert(properties != nullptr);
    return properties;
}


void Material::clearRessources() {
    resetQueue.flush();
    mainDeletionQueue.flush();
    if (material_buffer.handle != VK_NULL_HANDLE)
        context->resource_builder->destroyBuffer(material_buffer);
}

void Material::reset() {
    resetQueue.flush();
}
