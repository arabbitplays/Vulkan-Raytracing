//
// Created by oschdi on 12/14/24.
//

#include "TopLevelAccelerationStructureBuilder.hpp"

VkTransformMatrixKHR convertToVkTransform(const glm::mat4& mat) {
    VkTransformMatrixKHR transform{};

    // Copy first three rows from glm::mat4 to VkTransformMatrixKHR
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 4; ++col) {
            transform.matrix[row][col] = mat[col][row];  // Column-major order
        }
    }

    return transform;
}

void TopLevelAccelerationStructureBuilder::addInstance(AccelerationStructure blas, glm::mat4 transform_matrix) {
    VkAccelerationStructureInstanceKHR accelerationStructureInstance{};
    accelerationStructureInstance.transform = convertToVkTransform(transform_matrix);
    accelerationStructureInstance.instanceCustomIndex = 0;
    accelerationStructureInstance.mask = 0xFF;
    accelerationStructureInstance.instanceShaderBindingTableRecordOffset = 0;
    accelerationStructureInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    accelerationStructureInstance.accelerationStructureReference = blas.deviceAddress; // TODO verallgemeinern
    instances.push_back(accelerationStructureInstance);
}

void TopLevelAccelerationStructureBuilder::fillInstanceBuffer() {
    uint32_t instance_data_size = instances.size() * sizeof(VkAccelerationStructureInstanceKHR);

    if (instanceBuffers.size() == 0) { // TODO Make this only one
        AllocatedBuffer instanceBuffer = ressource_builder.createBuffer(
            instance_data_size,
            VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        instanceBuffers.push_back(instanceBuffer);
    }

    instanceBuffers[0].update(device, instances.data(), instance_data_size);
}

void TopLevelAccelerationStructureBuilder::addInstanceGeometry() {
    fillInstanceBuffer();

    VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
    instanceDataDeviceAddress.deviceAddress = instanceBuffers[0].deviceAddress;

    VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
    accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
    accelerationStructureGeometry.geometry.instances.data = instanceDataDeviceAddress;

    Geometry geometry{};
    geometry.handle = accelerationStructureGeometry;
    geometry.primitiveCount = static_cast<uint32_t>(instances.size());
    geometries.push_back(geometry);

    instances.clear();
}

void TopLevelAccelerationStructureBuilder::update_instance_geometry(uint32_t index) {
    fillInstanceBuffer();

    VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
    instanceDataDeviceAddress.deviceAddress = instanceBuffers[0].deviceAddress;

    VkAccelerationStructureGeometryKHR accelerationStructureGeometry = geometries[index].handle;
    accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
    accelerationStructureGeometry.geometry.instances.data = instanceDataDeviceAddress;

    geometries[index].primitiveCount = static_cast<uint32_t>(instances.size());
    geometries[index].updated = true;

    instances.clear();
}
