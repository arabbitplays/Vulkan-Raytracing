//
// Created by oster on 07.09.2024.
//

#ifndef BASICS_MESHASSETBUILDER_HPP
#define BASICS_MESHASSETBUILDER_HPP


#include <AccelerationStructure.hpp>
#include <MeshAsset.hpp>
#include "RessourceBuilder.hpp"

class MeshAssetBuilder {
public:
    MeshAssetBuilder() = default;
    MeshAssetBuilder(VkDevice device, RessourceBuilder ressource_builder);
    MeshAsset LoadMeshAsset(std::string name, std::string path);
    void destroyMeshAsset(MeshAsset& meshAsset);

private:

    VkDevice device;
    RessourceBuilder ressource_builder;
};


#endif //BASICS_MESHASSETBUILDER_HPP
