//
// Created by oster on 07.09.2024.
//

#include <stdexcept>
#include <tiny_obj_loader.h>
#include <iostream>
#include "MeshAssetBuilder.hpp"

#include <cstring>

MeshAssetBuilder::MeshAssetBuilder(VkDevice device, RessourceBuilder bufferBuilder) {
    this->device = device;
    this->bufferBuilder = bufferBuilder;
}

MeshAsset MeshAssetBuilder::LoadMeshAsset(std::string name, std::string path) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    loadModel(path, vertices, indices);

    MeshBuffers meshBuffers{};
    meshBuffers.vertexBuffer = createVertexBuffer(vertices);
    meshBuffers.indexBuffer = createIndexBuffer(indices);

    MeshAsset meshAsset{};
    meshAsset.name = name;
    meshAsset.meshBuffers = meshBuffers;
    std::vector<MeshSurface> surfaces;
    surfaces.push_back({0, static_cast<uint32_t>(indices.size())});
    meshAsset.surfaces = surfaces;

    return meshAsset;
}

void MeshAssetBuilder::loadModel(std::string path, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())) {
        throw std::runtime_error(warn + err);
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
            };

            if (!attrib.texcoords.empty()) {
                vertex.texCoord = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };
            } else {
                vertex.texCoord = {0, 0};
            }

            vertex.color = {1.0f, 1.0f, 1.0f};
            vertex.normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]};

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVertices[vertex]);
        }
    }
}

AllocatedBuffer MeshAssetBuilder::createVertexBuffer(std::vector<Vertex>& vertices) {
    VkDeviceSize size = sizeof(vertices[0]) * vertices.size();

    AllocatedBuffer stagingBuffer = bufferBuilder.createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT
            , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* data;
    vkMapMemory(device, stagingBuffer.bufferMemory, 0, size, 0, &data);
    memcpy(data, vertices.data(), (size_t) size);
    vkUnmapMemory(device, stagingBuffer.bufferMemory);

    AllocatedBuffer vertexBuffer = bufferBuilder.createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    bufferBuilder.copyBuffer(stagingBuffer, vertexBuffer, size);

    bufferBuilder.destroyBuffer(stagingBuffer);
    return vertexBuffer;
}

AllocatedBuffer MeshAssetBuilder::createIndexBuffer(std::vector<uint32_t>& indices) {
    VkDeviceSize size = sizeof(indices[0]) * indices.size();

    AllocatedBuffer stagingBuffer = bufferBuilder.createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT
            , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* data;
    vkMapMemory(device, stagingBuffer.bufferMemory, 0, size, 0, &data);
    memcpy(data, indices.data(), (size_t) size);
    vkUnmapMemory(device, stagingBuffer.bufferMemory);

    AllocatedBuffer indexBuffer = bufferBuilder.createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    bufferBuilder.copyBuffer(stagingBuffer, indexBuffer, size);

    bufferBuilder.destroyBuffer(stagingBuffer);
    return indexBuffer;
}

void MeshAssetBuilder::destroyMeshAsset(MeshAsset& meshAsset) {
    bufferBuilder.destroyBuffer(meshAsset.meshBuffers.vertexBuffer);
    bufferBuilder.destroyBuffer(meshAsset.meshBuffers.indexBuffer);
}