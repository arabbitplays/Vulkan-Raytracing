//
// Created by oschdi on 1/8/25.
//

#include "ModelLoader.hpp"

#include <iostream>
#include <stdexcept>

MeshAsset ModelLoader::loadMeshAsset(std::string name, std::string resources_path, std::string path) {
    MeshBuffers meshBuffers{};

    std::string full_path = resources_path + "/" + path;
    loadData(full_path, meshBuffers.vertices, meshBuffers.indices);

    MeshAsset meshAsset{};
    meshAsset.name = name;
    meshAsset.path = path;
    meshAsset.meshBuffers = meshBuffers;
    meshAsset.vertex_count = meshBuffers.indices.size();
    meshAsset.triangle_count = meshBuffers.indices.size() / 3;

    meshAsset.instance_data = {};
    return meshAsset;
}
