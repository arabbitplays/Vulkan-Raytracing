#ifndef BASICS_MESHASSETBUILDER_HPP
#define BASICS_MESHASSETBUILDER_HPP


#include <AccelerationStructure.hpp>
#include <MeshAsset.hpp>
#include <ModelLoader.hpp>

#include "ResourceBuilder.hpp"

namespace RtEngine {
class MeshAssetBuilder {
public:
    MeshAssetBuilder() = default;
    MeshAssetBuilder(VkDevice device, const std::string& resource_path)
        : device(device), resource_path(resource_path) {};
    MeshAsset loadMeshAsset(std::string path);
    void destroyMeshAsset(MeshAsset& meshAsset);

private:

    VkDevice device;
    std::string resource_path;
};


}
#endif //BASICS_MESHASSETBUILDER_HPP
