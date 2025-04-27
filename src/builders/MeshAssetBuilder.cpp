#include <stdexcept>
#include <iostream>
#include "MeshAssetBuilder.hpp"

#include <AssimpModelLoader.hpp>
#include <cstring>
#include <ModelLoader.hpp>

namespace RtEngine {
MeshAsset MeshAssetBuilder::loadMeshAsset(std::string path) {
    AssimpModelLoader loader;
    return loader.loadMeshAsset(resource_path, path);
}

void MeshAssetBuilder::destroyMeshAsset(MeshAsset& meshAsset) {
    // intentional empty
}
}
