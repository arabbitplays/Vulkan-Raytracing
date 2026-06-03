#include "../../../include/engine/repositories/VolumeRepository.hpp"

namespace RtEngine {
    VolumeRepository::VolumeRepository(const std::shared_ptr<VulkanContext> &context, const std::string &resource_dir) {
        volume_builder = std::make_shared<VolumeBuilder>();
    }

    std::shared_ptr<Volume> VolumeRepository::getOrCreateVolume(const std::string &path) {
        if (!volume_path_cache.contains(path)) {
           volume_path_cache[path] = volume_builder->loadVolume(path);
        }
        return volume_path_cache[path];
    }

    std::shared_ptr<VolumeAsset> VolumeRepository::createHeterogenousVolumeAsset(const fs::path &path, glm::vec3 absorption, glm::vec3 scattering, float g, const std::shared_ptr<MeshAsset> &mesh_asset) {
        auto volume_asset = std::make_shared<VolumeAsset>();
        volume_asset->name = "_hetero_" + path.stem().string(); // TODO collect Volumes not VolumeAssets
        volume_asset->absorption_scale = absorption;
        volume_asset->scattering_scale = scattering;
        volume_asset->g = g;
        volume_asset->bounding_mesh = mesh_asset;
        volume_asset->volume = getOrCreateVolume(path);

        return volume_asset;
    }

    std::shared_ptr<VolumeAsset> VolumeRepository::createHomogenousVolumeAsset(glm::vec3 absorption, glm::vec3 scattering, float g, float majorant,
                                                           const std::shared_ptr<MeshAsset> &mesh_asset) {

        auto volume_asset = std::make_shared<VolumeAsset>();
        volume_asset->name = "_Homo_" + std::to_string(absorption.x) + "_" + std::to_string(scattering.x); // TODO eww what is this
        volume_asset->volume = VolumeBuilder::createHomogenous();
        volume_asset->absorption_scale = absorption;
        volume_asset->scattering_scale = scattering;
        volume_asset->bonus_majorant = majorant;
        volume_asset->g = g;
        volume_asset->bounding_mesh = mesh_asset;

        return volume_asset;
    }

    void VolumeRepository::destroy() {

    }
} // RtEngine