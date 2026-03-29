#ifndef VULKAN_RAYTRACING_OPENVDBVOLUMELOADER_HPP
#define VULKAN_RAYTRACING_OPENVDBVOLUMELOADER_HPP
#include <filesystem>
#include <memory>

#include "Volume.hpp"

namespace RtEngine {
    class OpenVdbVolumeLoader {
    public:
        OpenVdbVolumeLoader() = default;

        std::shared_ptr<Volume> loadVolume(const std::filesystem::path &path);
    };
} // rtEngine

#endif //VULKAN_RAYTRACING_OPENVDBVOLUMELOADER_HPP