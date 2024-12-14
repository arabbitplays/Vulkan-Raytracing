//
// Created by oschdi on 12/14/24.
//

#ifndef ACCELERATIONSTRUCTUREBUILDER_HPP
#define ACCELERATIONSTRUCTUREBUILDER_HPP

#include <vulkan/vulkan_core.h>
#include <vector>
#include "MeshAssetBuilder.hpp"

struct AccelerationStructure {
    VkAccelerationStructureKHR handle;
    uint64_t deviceAddress;
    AllocatedBuffer buffer;
};

class AccelerationStructureBuilder {
public:
    AccelerationStructureBuilder() = default;
    AccelerationStructureBuilder(VkDevice device, RessourceBuilder ressource_builder, CommandManager command_manager);

    AccelerationStructure buildBlasFromMesh(const MeshAsset& mesh);
    AccelerationStructure buildTlasFromMesh(std::vector<AccelerationStructure>& blas);
    AccelerationStructure buildAccelerationStructure(VkAccelerationStructureTypeKHR type, uint32_t primitiveCount,
                                                    std::vector<VkAccelerationStructureGeometryKHR>& geometries);

    void destroyAccelerationStructure(AccelerationStructure &accelerationStructure);

private:
    VkDevice device;
    RessourceBuilder ressource_builder;
    CommandManager command_manager;
};

#endif //ACCELERATIONSTRUCTUREBUILDER_HPP
