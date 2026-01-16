#ifndef VULKAN_RAYTRACING_MATERIALINSTANCE_HPP
#define VULKAN_RAYTRACING_MATERIALINSTANCE_HPP
#include "PropertiesManager.hpp"

struct MaterialInstance {
    std::shared_ptr<RtEngine::PropertiesSection> properties;
    uint32_t material_index;
};

#endif //VULKAN_RAYTRACING_MATERIALINSTANCE_HPP