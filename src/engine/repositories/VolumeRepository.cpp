#include "../../../include/engine/repositories/VolumeRepository.hpp"

namespace RtEngine {
    VolumeRepository::VolumeRepository(const std::shared_ptr<VulkanContext> &context, const std::string &resource_dir) {

    }

    std::shared_ptr<VolumeAsset> VolumeRepository::getVolume(const std::string &name) {
        auto volume = std::make_shared<VolumeAsset>();
        volume->name = name;
        volume->isHomogenous = true;
        volume->volume_data = { 0.01, 0.02 };
        return volume;
    }

    std::string VolumeRepository::addVolume(std::string path) {
        throw new std::runtime_error("Not implemented");
    }

    void VolumeRepository::destroy() {

    }
} // RtEngine