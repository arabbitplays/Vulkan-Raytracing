//
// Created by oschdi on 17.01.26.
//

#include "../../../../include/engine/renderer/vulkan_scene_representation/MaterialManager.hpp"

#include "SceneUtil.hpp"

namespace RtEngine {
    MaterialManager::MaterialManager(std::shared_ptr<ResourceBuilder> resource_builder,
        std::shared_ptr<TextureRepository> tex_repo) : resource_builder(resource_builder) {
        material_textures = std::make_shared<MaterialTextures<>>(tex_repo);
    }

    void MaterialManager::updateMaterialResources(std::shared_ptr<Scene> scene) {
        // clear all resources that have been created earlier
        if (material_buffer.handle != VK_NULL_HANDLE) {
            resource_builder->destroyBuffer(material_buffer);
        }
        material_textures->clear();

        // collect material instance from scene graph, combine their data resources into a buffer and the textures into material_textures
        std::vector<std::shared_ptr<MaterialInstance>> material_instances = SceneUtil::collectMaterialInstances(scene->getRootNode());
        material_buffer = createMaterialBuffer(material_instances);

        // write new resources
        std::shared_ptr<Material> material = scene->material;
        material->writeMaterial(material_buffer, material_textures);
    }

    AllocatedBuffer MaterialManager::createMaterialBuffer(const std::vector<std::shared_ptr<MaterialInstance>> &instances) const {
        std::vector<void*> resource_ptrs(instances.size());
        std::vector<size_t> sizes(instances.size());
        size_t total_size = 0;
        for (uint32_t i = 0; i < instances.size(); i++) {
            resource_ptrs[i] = instances[i]->getResources(&sizes[i], material_textures);
            instances[i]->setMaterialIndex(i);
            total_size += sizes[i];
        }

        const auto material_data = static_cast<std::byte*>(std::malloc(total_size));
        std::byte* dst = material_data;
        for (uint32_t i = 0; i < resource_ptrs.size(); i++) {
            std::memcpy(dst, resource_ptrs[i], sizes[i]);
            dst += sizes[i];
        }

        AllocatedBuffer material_buffer = resource_builder->stageMemoryToNewBuffer(
                material_data, total_size,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

        free(material_data);
        return material_buffer;
    }

    void MaterialManager::destroy() {
        if (material_buffer.handle != VK_NULL_HANDLE) {
            resource_builder->destroyBuffer(material_buffer);
        }
    }
} // RtEngine