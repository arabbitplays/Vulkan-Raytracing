//
// Created by oschdi on 1/8/25.
//

#include "ModelLoader.hpp"

#include <iostream>
#include <stdexcept>

MeshAsset ModelLoader::LoadMeshAsset(std::string name, std::string path) {
    MeshBuffers meshBuffers{};

    loadData(path, meshBuffers.vertices, meshBuffers.indices);

    MeshAsset meshAsset{};
    meshAsset.name = name;
    meshAsset.meshBuffers = meshBuffers;
    meshAsset.vertex_count = meshBuffers.indices.size();
    meshAsset.triangle_count = meshBuffers.indices.size() / 3;

    meshAsset.instance_data = {};
    return meshAsset;
}
