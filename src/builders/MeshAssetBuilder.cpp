//
// Created by oster on 07.09.2024.
//

#include <stdexcept>
#include <iostream>
#include "MeshAssetBuilder.hpp"

#include <AssimpModelLoader.hpp>
#include <cstring>
#include <ModelLoader.hpp>

MeshAsset MeshAssetBuilder::LoadMeshAsset(std::string name, std::string path) {
    AssimpModelLoader loader;
    return loader.loadMeshAsset(name, resource_path, path);
}

void MeshAssetBuilder::destroyMeshAsset(MeshAsset& meshAsset) {
    meshAsset.accelerationStructure->destroy();
}
