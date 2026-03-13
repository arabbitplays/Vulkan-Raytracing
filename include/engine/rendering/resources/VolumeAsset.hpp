#ifndef VULKAN_RAYTRACING_VOLUMEASSET_HPP
#define VULKAN_RAYTRACING_VOLUMEASSET_HPP
#include <cstdint>
#include <vector>

namespace RtEngine {
    struct VolumeData {
        float absorption;
        float scattering;
    };

    struct VolumeAsset {
        std::string name;
        uint32_t volume_id;
        bool isHomogenous = true;
        VolumeData volume_data;
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_VOLUMEASSET_HPP