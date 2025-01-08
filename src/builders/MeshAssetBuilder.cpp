//
// Created by oster on 07.09.2024.
//

#include <stdexcept>
#include <iostream>
#include "MeshAssetBuilder.hpp"

#include <AssimpModelLoader.hpp>
#include <cstring>
#include <IRenderable.hpp>
#include <ModelLoader.hpp>

MeshAssetBuilder::MeshAssetBuilder(VkDevice device, RessourceBuilder ressource_builder) {
    this->device = device;
    this->ressource_builder = ressource_builder;
}

MeshAsset MeshAssetBuilder::LoadMeshAsset(std::string name, std::string path) {
    AssimpModelLoader loader;
    return loader.LoadMeshAsset(name, path);
}

AllocatedBuffer MeshAssetBuilder::createVertexBuffer(std::vector<std::shared_ptr<MeshAsset>>& mesh_assets) {
    assert(!mesh_assets.empty());

    VkDeviceSize size =  0;
    for (auto& mesh_asset : mesh_assets) {
        size += mesh_asset->meshBuffers.vertices.size() * sizeof(mesh_asset->meshBuffers.vertices[0]);;
    }

    AllocatedBuffer stagingBuffer = ressource_builder.createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT
            , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* data;
    vkMapMemory(device, stagingBuffer.bufferMemory, 0, size, 0, &data);
    uint32_t offset = 0;
    uint32_t vertex_offset = 0;
    for (auto& mesh_asset : mesh_assets) {
        uint32_t mesh_size = mesh_asset->meshBuffers.vertices.size() * sizeof(mesh_asset->meshBuffers.vertices[0]);
        memcpy(data + offset, mesh_asset->meshBuffers.vertices.data(), (size_t) mesh_size);

        mesh_asset->instance_data.vertex_offset = vertex_offset;
        offset += mesh_size;
        vertex_offset += mesh_asset->meshBuffers.vertices.size();
    }
    vkUnmapMemory(device, stagingBuffer.bufferMemory);

    AllocatedBuffer vertexBuffer = ressource_builder.createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
        | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    ressource_builder.copyBuffer(stagingBuffer, vertexBuffer, size);

    ressource_builder.destroyBuffer(stagingBuffer);
    return vertexBuffer;
}

AllocatedBuffer MeshAssetBuilder::createIndexBuffer(std::vector<std::shared_ptr<MeshAsset>>& mesh_assets) {
    assert(!mesh_assets.empty());

    VkDeviceSize size =  0;
    for (auto& mesh_asset : mesh_assets) {
        size += mesh_asset->meshBuffers.indices.size() * sizeof(mesh_asset->meshBuffers.indices[0]);;
    }


    AllocatedBuffer stagingBuffer = ressource_builder.createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT
            , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* data;
    vkMapMemory(device, stagingBuffer.bufferMemory, 0, size, 0, &data);
    uint32_t offset = 0;
    uint32_t index_offset = 0;
    for (auto& mesh_asset : mesh_assets) {
        uint32_t mesh_size = mesh_asset->meshBuffers.indices.size() * sizeof(mesh_asset->meshBuffers.indices[0]);
        memcpy(data + offset, mesh_asset->meshBuffers.indices.data(), (size_t) mesh_size);

        mesh_asset->instance_data.triangle_offset = index_offset;
        offset += mesh_size;
        index_offset += mesh_asset->meshBuffers.indices.size();
    }
    vkUnmapMemory(device, stagingBuffer.bufferMemory);

    AllocatedBuffer indexBuffer = ressource_builder.createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
        | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    ressource_builder.copyBuffer(stagingBuffer, indexBuffer, size);

    ressource_builder.destroyBuffer(stagingBuffer);
    return indexBuffer;
}

AllocatedBuffer MeshAssetBuilder::createGeometryMappingBuffer(std::vector<std::shared_ptr<MeshAsset>>& mesh_assets) {
    assert(!mesh_assets.empty());

    std::vector<GeometryData> geometry_datas;
    for (auto& mesh_asset : mesh_assets) {
        geometry_datas.push_back(mesh_asset->instance_data);
    }
    return ressource_builder.stageMemoryToNewBuffer(geometry_datas.data(), mesh_assets.size() * sizeof(GeometryData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

AllocatedBuffer MeshAssetBuilder::createInstanceMappingBuffer(std::vector<RenderObject>& objects) {
    assert(!objects.empty());

    std::vector<InstanceData> instance_datas;
    for (auto& object : objects) {
        instance_datas.push_back(object.instance_data);
    }
    return ressource_builder.stageMemoryToNewBuffer(instance_datas.data(), instance_datas.size() * sizeof(InstanceData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

void MeshAssetBuilder::destroyMeshAsset(MeshAsset& meshAsset) {
    meshAsset.accelerationStructure->destroy();
}
