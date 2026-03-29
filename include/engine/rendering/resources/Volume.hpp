#ifndef VULKAN_RAYTRACING_VOLUME_HPP
#define VULKAN_RAYTRACING_VOLUME_HPP
#include <filesystem>
#include <memory>
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

struct Volume {
    std::filesystem::path path;
    glm::uvec3 size;
    std::vector<float> densities;
    float max_density;
};

#endif //VULKAN_RAYTRACING_VOLUME_HPP