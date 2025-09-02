//
// Created by oschdi on 02.09.25.
//

#ifndef VULKAN_RAYTRACING_MATERIALREPOSITORY_HPP
#define VULKAN_RAYTRACING_MATERIALREPOSITORY_HPP
#include <future>

#include "Material.hpp"

namespace RtEngine {
    class MaterialRepository {
    public:
        MaterialRepository() = default;

        void addMaterial(const std::shared_ptr<Material> &material);
        std::shared_ptr<Material> getMaterial(const std::string &name);
        void resetMaterials() const;
        void destroyMaterials() const;
    private:
        std::unordered_map<std::string, std::shared_ptr<Material>> materials;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_MATERIALREPOSITORY_HPP