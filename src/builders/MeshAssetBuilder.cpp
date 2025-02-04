//
// Created by oster on 07.09.2024.
//

#include <stdexcept>
#include <iostream>
#include "MeshAssetBuilder.hpp"

#include <AssimpModelLoader.hpp>
#include <cstring>
#include <ModelLoader.hpp>

// TODO what is this class???
MeshAssetBuilder::MeshAssetBuilder(VkDevice device, RessourceBuilder ressource_builder) {
    this->device = device;
    this->ressource_builder = ressource_builder;
}

MeshAsset MeshAssetBuilder::LoadMeshAsset(std::string name, std::string path) {
    AssimpModelLoader loader;
    return loader.LoadMeshAsset(name, path);
}

void MeshAssetBuilder::destroyMeshAsset(MeshAsset& meshAsset) {
    meshAsset.accelerationStructure->destroy();
}
