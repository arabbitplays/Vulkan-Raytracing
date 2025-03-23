//
// Created by oster on 07.09.2024.
//

#include <stdexcept>
#include <iostream>
#include "MeshAssetBuilder.hpp"

#include <AssimpModelLoader.hpp>
#include <cstring>
#include <ModelLoader.hpp>

MeshAsset MeshAssetBuilder::loadMeshAsset(std::string path) {
    AssimpModelLoader loader;
    return loader.loadMeshAsset(resource_path, path);
}

void MeshAssetBuilder::destroyMeshAsset(MeshAsset& meshAsset) {
    // intentional empty
}
