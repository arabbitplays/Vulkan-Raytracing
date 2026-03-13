#ifndef VULKAN_RAYTRACING_VOLUMEASSET_HPP
#define VULKAN_RAYTRACING_VOLUMEASSET_HPP
#include <cstdint>
#include <vector>

namespace RtEngine {
    struct VolumeAsset {
        bool isHomogenous = true;
        uint32_t voxel_count = 0;
        std::vector<float> voxel_densities;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_VOLUMEASSET_HPP