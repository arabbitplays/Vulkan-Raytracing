//
// Created by oster on 07.09.2024.
//

#ifndef BASICS_MESHASSETBUILDER_HPP
#define BASICS_MESHASSETBUILDER_HPP


#include <AccelerationStructure.hpp>
#include <IRenderable.hpp>
#include <MeshAsset.hpp>
#include <vulkan/vulkan.h>
#include <vector>
#include "../rendering/engine/CommandManager.hpp"
#include "../rendering/Vertex.hpp"
#include "RessourceBuilder.hpp"

class MeshAssetBuilder {
public:
    MeshAssetBuilder() = default;
    MeshAssetBuilder(VkDevice device, RessourceBuilder ressource_builder);
    MeshAsset LoadMeshAsset(std::string name, std::string path);
    void destroyMeshAsset(MeshAsset& meshAsset);

    AllocatedBuffer createVertexBuffer(std::vector<std::shared_ptr<MeshAsset>>& mesh_assets);
    AllocatedBuffer createIndexBuffer(std::vector<std::shared_ptr<MeshAsset>>& mesh_assets);

    AllocatedBuffer createGeometryMappingBuffer(std::vector<std::shared_ptr<MeshAsset>>& mesh_assets);

    AllocatedBuffer createInstanceMappingBuffer(std::vector<RenderObject> &objects);

private:
    void loadModel(std::string path, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);


    VkDevice device;
    RessourceBuilder ressource_builder;
};


#endif //BASICS_MESHASSETBUILDER_HPP
