#ifndef MESHREPOSITORY_HPP
#define MESHREPOSITORY_HPP
#include <DeletionQueue.hpp>
#include <MeshAssetBuilder.hpp>

#include <memory>

namespace RtEngine {
class VulkanContext;

class MeshRepository {
public:
    MeshRepository() = default;
    MeshRepository(const std::shared_ptr<VulkanContext>& context);

    std::shared_ptr<MeshAsset> getMesh(const std::string& name);
    std::string addMesh(std::string path);
    void destroy();
private:
    std::shared_ptr<MeshAssetBuilder> mesh_asset_builder;
    DeletionQueue deletion_queue;

    std::unordered_map<std::string, std::shared_ptr<MeshAsset>> mesh_name_cache, mesh_path_cache;
};



}
#endif //MESHREPOSITORY_HPP
