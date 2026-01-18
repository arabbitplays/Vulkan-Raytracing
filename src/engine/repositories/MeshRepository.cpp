#include "MeshRepository.hpp"

#include <VulkanContext.hpp>
#include <spdlog/spdlog.h>

namespace RtEngine {
	MeshRepository::MeshRepository(const std::shared_ptr<VulkanContext> &context, const std::string& resource_dir) {
		mesh_asset_builder = std::make_shared<MeshAssetBuilder>(context->device_manager->getDevice(),
																resource_dir);
	}

	std::shared_ptr<MeshAsset> MeshRepository::getMesh(const std::string &name) {
		if (mesh_name_cache.contains(name)) {
			return mesh_name_cache[name];
		}
		throw std::runtime_error("Mesh '" + name + "' does not exist");
	}

	// returns the name given to the mesh
	std::string MeshRepository::addMesh(std::string path) {
		if (mesh_path_cache.contains(path)) {
			spdlog::debug("Mesh cache hit with path: {}", path);
			return mesh_path_cache[path]->name;
		}

		MeshAsset mesh_asset = mesh_asset_builder->loadMeshAsset(path);
		mesh_name_cache[mesh_asset.name] = std::make_shared<MeshAsset>(mesh_asset);
		mesh_path_cache[path] = mesh_name_cache[mesh_asset.name];
		return mesh_asset.name;
	}

	void MeshRepository::destroy() {
		deletion_queue.flush();

		for (auto &mesh: mesh_name_cache) {
			mesh_asset_builder->destroyMeshAsset(*mesh.second);
		}
	}
} // namespace RtEngine
