//
// Created by oster on 07.09.2024.
//

#ifndef BASICS_MESHASSETBUILDER_HPP
#define BASICS_MESHASSETBUILDER_HPP


#include <AccelerationStructure.hpp>
#include <MeshAsset.hpp>
#include <ModelLoader.hpp>

#include "RessourceBuilder.hpp"

class MeshAssetBuilder {
public:
    MeshAssetBuilder() = default;
    MeshAssetBuilder(VkDevice device, RessourceBuilder ressource_builder, const std::string& resource_path)
        : device(device), ressource_builder(ressource_builder), resource_path(resource_path) {};
    MeshAsset loadMeshAsset(std::string path);
    void destroyMeshAsset(MeshAsset& meshAsset);

private:

    VkDevice device;
    RessourceBuilder ressource_builder;
    std::string resource_path;
};


#endif //BASICS_MESHASSETBUILDER_HPP
