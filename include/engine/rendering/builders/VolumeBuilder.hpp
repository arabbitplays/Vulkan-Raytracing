#ifndef VULKAN_RAYTRACING_VOLUMEASSETBUILDER_HPP
#define VULKAN_RAYTRACING_VOLUMEASSETBUILDER_HPP
#include "Volume.hpp"

namespace RtEngine {
    class VolumeBuilder {
    public:
        std::shared_ptr<Volume> loadVolume(const std::filesystem::path &path);
        static std::shared_ptr<Volume> createHomogenous();
    };
} // RtEngine

#endif //VULKAN_RAYTRACING_VOLUMEASSETBUILDER_HPP