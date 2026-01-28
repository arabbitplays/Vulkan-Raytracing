#ifndef VULKAN_RAYTRACING_MATERIALMANAGER_HPP
#define VULKAN_RAYTRACING_MATERIALMANAGER_HPP

#include "IScene.hpp"
#include "MaterialInstance.hpp"
#include "MaterialTextures.hpp"

namespace RtEngine {
    class MaterialManager {
    public:
        MaterialManager(std::shared_ptr<ResourceBuilder> resource_builder, std::shared_ptr<TextureRepository> tex_repo);

        void updateMaterialResources(std::shared_ptr<IScene> scene);

        AllocatedBuffer createMaterialBuffer(const std::vector<std::shared_ptr<MaterialInstance>> &instances) const;

        void destroy();

    private:
        std::shared_ptr<ResourceBuilder> resource_builder;

        AllocatedBuffer material_buffer;
        std::shared_ptr<MaterialTextures<>> material_textures;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_MATERIALMANAGER_HPP