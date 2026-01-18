#ifndef VULKAN_RAYTRACING_ISCENEMANAGER_HPP
#define VULKAN_RAYTRACING_ISCENEMANAGER_HPP
#include <memory>

#include "Material.hpp"

namespace RtEngine {
    class ISceneManager {
    public:
        virtual ~ISceneManager() = default;

        virtual std::shared_ptr<Material> getCurrentMaterial() = 0;
    };
}

#endif //VULKAN_RAYTRACING_ISCENEMANAGER_HPP