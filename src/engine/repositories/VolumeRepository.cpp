#include "../../../include/engine/repositories/VolumeRepository.hpp"

namespace RtEngine {
    VolumeRepository::VolumeRepository(const std::shared_ptr<VulkanContext> &context, const std::string &resource_dir) {
        volume_builder = std::make_shared<VolumeBuilder>();
    }

    std::shared_ptr<VolumeAsset> VolumeRepository::getVolume(const std::string &name) {
        if (!volume_asset_cache.contains(name)) {
            throw new std::runtime_error("Volume asset " + name + " isn ot loaded");
        }
        return volume_asset_cache[name];
    }

    std::string VolumeRepository::addVolumeAsset(const fs::path &path, float absorption, float scattering, float g, const std::shared_ptr<MeshAsset> &mesh_asset) {
        auto volume_asset = std::make_shared<VolumeAsset>();
        volume_asset->name = std::to_string(volume_asset_cache.size()) + "_hetero_" + path.stem().string();
        volume_asset->absorption_scale = absorption;
        volume_asset->scattering_scale = scattering;
        volume_asset->g = g;
        volume_asset->bounding_mesh = mesh_asset;

        if (!volume_path_cache.contains(path)) {
           volume_path_cache[path] = volume_builder->loadVolume(path);
        }
        volume_asset->volume = volume_path_cache[path];

        volume_asset_cache[volume_asset->name] = volume_asset;
        return volume_asset->name;
    }

    std::string VolumeRepository::addHomogenousVolumeAsset(float absorption, float scattering, float g, float majorant,
                                                           const std::shared_ptr<MeshAsset> &mesh_asset) {

        auto volume_asset = std::make_shared<VolumeAsset>();
        volume_asset->name = std::to_string(volume_asset_cache.size()) + "_Homo_" + std::to_string(absorption) + "_" + std::to_string(scattering);
        volume_asset->volume = VolumeBuilder::createHomogenous();
        volume_asset->absorption_scale = absorption;
        volume_asset->scattering_scale = scattering;
        volume_asset->g = g;
        volume_asset->bounding_mesh = mesh_asset;

        volume_asset_cache[volume_asset->name] = volume_asset;
        return volume_asset->name;
    }

    void VolumeRepository::destroy() {

    }
} // RtEngine