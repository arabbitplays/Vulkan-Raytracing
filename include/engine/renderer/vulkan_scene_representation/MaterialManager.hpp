//
// Created by oschdi on 17.01.26.
//

#ifndef VULKAN_RAYTRACING_MATERIALMANAGER_HPP
#define VULKAN_RAYTRACING_MATERIALMANAGER_HPP
#include "MaterialInstance.hpp"
#include "MaterialTextures.hpp"
#include "Scene.hpp"

namespace RtEngine {
    class MaterialManager {
    public:
        MaterialManager(std::shared_ptr<ResourceBuilder> resource_builder, std::shared_ptr<TextureRepository> tex_repo);

        void updateMaterialResources(std::shared_ptr<Scene> scene);

        AllocatedBuffer createMaterialBuffer(const std::vector<std::shared_ptr<MaterialInstance>> &instances) const;

        void destroy();

    private:
        std::shared_ptr<ResourceBuilder> resource_builder;

        AllocatedBuffer material_buffer;
        std::shared_ptr<MaterialTextures<>> material_textures;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_MATERIALMANAGER_HPP