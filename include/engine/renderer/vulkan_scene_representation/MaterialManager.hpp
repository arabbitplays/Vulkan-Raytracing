//
// Created by oschdi on 17.01.26.
//

#ifndef VULKAN_RAYTRACING_MATERIALMANAGER_HPP
#define VULKAN_RAYTRACING_MATERIALMANAGER_HPP
#include "MaterialInstance.hpp"
#include "MaterialTextures.hpp"

namespace RtEngine {
    class MaterialManager {
    public:
        MaterialManager() = default;

    private:
        std::unordered_map<std::string, MaterialInstance> material_instances;

        std::shared_ptr<MaterialTextures<>> tex_repo;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_MATERIALMANAGER_HPP