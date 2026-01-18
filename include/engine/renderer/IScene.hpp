//
// Created by oschdi on 18.01.26.
//

#ifndef VULKAN_RAYTRACING_ISCENE_HPP
#define VULKAN_RAYTRACING_ISCENE_HPP
#include "EnvironmentMap.hpp"
#include "MeshAsset.hpp"
#include "MaterialInstance.hpp"
#include "IRenderable.hpp"

namespace RtEngine {
    class IScene {
    public:
        virtual ~IScene() = default;

        virtual std::vector<std::shared_ptr<MeshAsset>> getMeshAssets() = 0;
        virtual std::vector<std::shared_ptr<MaterialInstance>> getMaterialInstances() = 0;
        virtual void fillDrawContext(const std::shared_ptr<DrawContext> &draw_context) = 0;
        virtual std::shared_ptr<Material> getMaterial() = 0;
        virtual std::shared_ptr<EnvironmentMap> getEnvironmentMap() = 0;
        virtual void* getSceneData(size_t *size, uint32_t emitting_instances_count) = 0;
    };
}


#endif //VULKAN_RAYTRACING_ISCENE_HPP