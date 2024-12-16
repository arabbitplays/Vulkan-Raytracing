//
// Created by oster on 07.09.2024.
//

#ifndef BASICS_MESHASSETBUILDER_HPP
#define BASICS_MESHASSETBUILDER_HPP


#include <AccelerationStructure.hpp>
#include <MeshAsset.hpp>
#include <vulkan/vulkan.h>
#include <vector>
#include "../rendering/engine/CommandManager.hpp"
#include "../rendering/Vertex.hpp"
#include "RessourceBuilder.hpp"

class MeshAssetBuilder {
public:
    MeshAssetBuilder() = default;
    MeshAssetBuilder(VkDevice device, RessourceBuilder bufferBuilder);
    MeshAsset LoadMeshAsset(std::string name, std::string path);
    void destroyMeshAsset(MeshAsset& meshAsset);

private:
    void loadModel(std::string path, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
    AllocatedBuffer createVertexBuffer(std::vector<Vertex>& vertices);
    AllocatedBuffer createIndexBuffer(std::vector<uint32_t>& indices);

    VkDevice device;
    RessourceBuilder bufferBuilder;
};


#endif //BASICS_MESHASSETBUILDER_HPP
