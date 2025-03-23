//
// Created by oschdi on 3/23/25.
//

#include "MeshRepository.hpp"

#include <spdlog/spdlog.h>

MeshRepository::MeshRepository(std::shared_ptr<MeshAssetBuilder>& mesh_asset_builder) : mesh_asset_builder(mesh_asset_builder) {}

std::shared_ptr<MeshAsset> MeshRepository::getMesh(const std::string& name)
{
    if (mesh_name_cache.contains(name))
    {
        return mesh_name_cache[name];
    }
    throw std::runtime_error("Mesh '" + name + "' does not exist");
}

// returns the name given to the mesh
std::string MeshRepository::addMesh(std::string path)
{
    if (mesh_path_cache.contains(path))
    {
        spdlog::debug("Mesh cache hit with path: {}", path);
        return mesh_path_cache[path]->name;
    }

    MeshAsset mesh_asset = mesh_asset_builder->loadMeshAsset(path);
    mesh_name_cache[mesh_asset.name] = std::make_shared<MeshAsset>(mesh_asset);
    mesh_path_cache[path] = mesh_name_cache[mesh_asset.name];
    return mesh_asset.name;
}

void MeshRepository::destroy()
{
    deletion_queue.flush();

    for (auto& mesh : mesh_name_cache)
    {
        mesh_asset_builder->destroyMeshAsset(*mesh.second);
    }
}
