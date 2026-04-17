#include "../../../../include/engine/renderer/builders/VolumeBuilder.hpp"

#include "OpenVdbVolumeLoader.hpp"

namespace RtEngine {
    std::shared_ptr<Volume> VolumeBuilder::loadVolume(const std::filesystem::path &path) {
        OpenVdbVolumeLoader loader;
        return loader.loadVolume(path);
    }

    std::shared_ptr<Volume> VolumeBuilder::createHomogenous() {
        auto result = std::make_shared<Volume>();
        result->path = "";
        result->max_density = 1;

        result->size = glm::uvec3(1);
        result->densities = std::vector<float>(1, 1);
        return result;
    }
} // RtEngine