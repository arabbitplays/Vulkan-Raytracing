//
// Created by oschdi on 12/15/24.
//

#ifndef ACCELERATIONSTRUCTURE_HPP
#define ACCELERATIONSTRUCTURE_HPP

#include <MeshAssetBuilder.hpp>
#include <RessourceBuilder.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>

struct Geometry {
    VkAccelerationStructureGeometryKHR handle;
    uint32_t primitiveCount;
    bool updated = false;
};

class AccelerationStructure {
public:
    AccelerationStructure() = default;
    AccelerationStructure(VkDevice& device, RessourceBuilder& ressource_builder, CommandManager& command_manager, VkAccelerationStructureTypeKHR type) :
        device(device), ressource_builder(ressource_builder), command_manager(command_manager), type(type) {};

    void addTriangleGeometry(const MeshAsset& mesh);

    void addInstance(std::shared_ptr<AccelerationStructure>& instance, glm::mat4 transform_matrix);
    void addInstanceGeometry();
    void update_instance_geometry(uint32_t index);

    void build(VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        VkBuildAccelerationStructureModeKHR mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR);

    void destroy();

    VkAccelerationStructureKHR getHandle() const;
    uint64_t getDeviceAddress() const;

private:
    void fillInstanceBuffer();

    VkDevice device;
    RessourceBuilder ressource_builder;
    CommandManager command_manager;

    VkAccelerationStructureTypeKHR type;

    VkAccelerationStructureKHR handle = VK_NULL_HANDLE;
    AllocatedBuffer buffer;
    uint64_t device_address;

    AllocatedBuffer instance_buffer;
    std::vector<VkAccelerationStructureInstanceKHR> instances{};
    std::vector<Geometry> geometries{};

};



#endif //ACCELERATIONSTRUCTURE_HPP