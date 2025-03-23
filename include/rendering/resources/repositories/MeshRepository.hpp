//
// Created by oschdi on 3/23/25.
//

#ifndef MESHREPOSITORY_HPP
#define MESHREPOSITORY_HPP
#include <DeletionQueue.hpp>
#include <MeshAssetBuilder.hpp>

#include <memory>


class MeshRepository {
public:
    MeshRepository() = default;
    MeshRepository(std::shared_ptr<MeshAssetBuilder>& mesh_asset_builder);

    std::shared_ptr<MeshAsset> getMesh(const std::string& name);
    std::string addMesh(std::string path);
    void destroy();
private:
    std::shared_ptr<MeshAssetBuilder> mesh_asset_builder;
    DeletionQueue deletion_queue;

    std::unordered_map<std::string, std::shared_ptr<MeshAsset>> mesh_name_cache, mesh_path_cache;
};



#endif //MESHREPOSITORY_HPP
