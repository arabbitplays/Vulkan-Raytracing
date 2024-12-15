//
// Created by oschdi on 12/14/24.
//

#ifndef ACCELERATIONSTRUCTUREBUILDER_HPP
#define ACCELERATIONSTRUCTUREBUILDER_HPP

#include <DeletionQueue.hpp>
#include <vulkan/vulkan_core.h>
#include <vector>
#include "MeshAssetBuilder.hpp"

struct AccelerationStructure {
    VkAccelerationStructureKHR handle = VK_NULL_HANDLE;
    uint64_t deviceAddress = 0;
    AllocatedBuffer buffer;
};

struct Geometry {
    VkAccelerationStructureGeometryKHR handle;
    uint32_t primitiveCount;
    bool updated = false;
};

class AccelerationStructureBuilder {
public:
    AccelerationStructureBuilder() = default;
    AccelerationStructureBuilder(VkDevice& device, RessourceBuilder& ressource_builder, CommandManager& command_manager);

    AccelerationStructure buildAccelerationStructure(VkAccelerationStructureTypeKHR type,
        VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        VkBuildAccelerationStructureModeKHR mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR);

    void destroyAccelerationStructure(AccelerationStructure &accelerationStructure);

protected:
    VkDevice device;
    AccelerationStructure acceleration_structure{};
    RessourceBuilder ressource_builder;
    CommandManager command_manager;
    std::vector<AllocatedBuffer> instanceBuffers{};
    std::vector<Geometry> geometries{};

};

#endif //ACCELERATIONSTRUCTUREBUILDER_HPP
