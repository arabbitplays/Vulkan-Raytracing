//
// Created by oschdi on 12/15/24.
//

#include "AccelerationStructure.hpp"

#include <cassert>
#include <stdexcept>

void GetAccelerationStructureBuildSizesKHR(VkDevice device,
                                           VkAccelerationStructureBuildTypeKHR buildType,
                                           const VkAccelerationStructureBuildGeometryInfoKHR* pBuildInfo,
                                           const uint32_t* pMaxPrimitiveCounts,
                                           VkAccelerationStructureBuildSizesInfoKHR* pSizeInfo) {
    auto func = (PFN_vkGetAccelerationStructureBuildSizesKHR) vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR");
    if (func != nullptr) {
        return func(device, buildType, pBuildInfo, pMaxPrimitiveCounts, pSizeInfo);
    }
}

VkResult CreateAccelerationStructureKHR(VkDevice device, const VkAccelerationStructureCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkAccelerationStructureKHR* pAccelerationStructure) {
    auto func = (PFN_vkCreateAccelerationStructureKHR) vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR");
    if (func != nullptr) {
        return func(device, pCreateInfo, pAllocator, pAccelerationStructure);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void CmdBuildAccelerationStructuresKHR(VkDevice device, VkCommandBuffer commandBuffer, uint32_t infoCount,
    const VkAccelerationStructureBuildGeometryInfoKHR* pInfos,
    const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos) {

    auto func = (PFN_vkCmdBuildAccelerationStructuresKHR) vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR");
    if (func != nullptr) {
        return func(commandBuffer, infoCount, pInfos, ppBuildRangeInfos);
    }
}

void DestroyAccelerationStructureKHR(VkDevice device, VkAccelerationStructureKHR accelerationStructure, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyAccelerationStructureKHR) vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR");
    if (func != nullptr) {
        return func(device, accelerationStructure, pAllocator);
    }
}

VkDeviceAddress GetAccelerationStructureDeviceAddressKHR( VkDevice device, const VkAccelerationStructureDeviceAddressInfoKHR* pInfo) {
    auto func = (PFN_vkGetAccelerationStructureDeviceAddressKHR) vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR");
    if (func != nullptr) {
        return func(device, pInfo);
    } else {
        return 0;
    }
}

// -----------------------------------------------------------------------------------------------------------------------

void AccelerationStructure::addTriangleGeometry(const AllocatedBuffer& vertex_buffer, const AllocatedBuffer& index_buffer,
        uint32_t max_vertex, uint32_t triangle_count, uint32_t vertex_stride, uint32_t vertex_offset, uint32_t index_offset) {
    assert(type == VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR);

    VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
    accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    accelerationStructureGeometry.geometry.triangles.vertexData.deviceAddress = vertex_buffer.deviceAddress + vertex_offset * vertex_stride;
    accelerationStructureGeometry.geometry.triangles.maxVertex = max_vertex;
    accelerationStructureGeometry.geometry.triangles.vertexStride = vertex_stride;
    accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
    accelerationStructureGeometry.geometry.triangles.indexData.deviceAddress = index_buffer.deviceAddress + index_offset * sizeof(uint32_t);

    Geometry geometry{};
    geometry.handle = accelerationStructureGeometry;
    geometry.primitiveCount = triangle_count;
    geometries.push_back(geometry);
}

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

void AccelerationStructure::addInstance(std::shared_ptr<AccelerationStructure>& instance, glm::mat4 transform_matrix, uint32_t instanceId) {
    VkAccelerationStructureInstanceKHR accelerationStructureInstance{};
    accelerationStructureInstance.transform = convertToVkTransform(transform_matrix);
    accelerationStructureInstance.instanceCustomIndex = instanceId;
    accelerationStructureInstance.mask = 0xFF;
    accelerationStructureInstance.instanceShaderBindingTableRecordOffset = 0;
    //accelerationStructureInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    accelerationStructureInstance.accelerationStructureReference = instance->getDeviceAddress();
    instances.push_back(accelerationStructureInstance);
}

void AccelerationStructure::fillInstanceBuffer() {
    uint32_t instance_data_size = instances.size() * sizeof(VkAccelerationStructureInstanceKHR);

    if (instance_buffer.handle == VK_NULL_HANDLE) {
        instance_buffer = ressource_builder.createBuffer(
            instance_data_size,
            VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    instance_buffer.update(device, instances.data(), instance_data_size);
}

void AccelerationStructure::addInstanceGeometry() {
    assert(type == VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR);

    fillInstanceBuffer();

    VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
    instanceDataDeviceAddress.deviceAddress = instance_buffer.deviceAddress;

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

void AccelerationStructure::update_instance_geometry(uint32_t index) {
    assert(type == VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR);
    fillInstanceBuffer();

    VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
    instanceDataDeviceAddress.deviceAddress = instance_buffer.deviceAddress;

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

void AccelerationStructure::build(VkBuildAccelerationStructureFlagsKHR flags, VkBuildAccelerationStructureModeKHR mode) {
    assert(!geometries.empty());

    std::vector<VkAccelerationStructureGeometryKHR> acceleration_structure_geometries;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> acceleration_structure_build_range_infos;
    std::vector<uint32_t> primitive_counts;
    for (auto geometry : geometries) {
        if (mode == VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR && !geometry.updated) {
            continue;
        }

        acceleration_structure_geometries.push_back(geometry.handle);

        VkAccelerationStructureBuildRangeInfoKHR accelerationBuildRangeInfo{};
        accelerationBuildRangeInfo.primitiveCount = geometry.primitiveCount;
        accelerationBuildRangeInfo.primitiveOffset = 0;
        accelerationBuildRangeInfo.firstVertex = 0;
        accelerationBuildRangeInfo.transformOffset = 0;
        acceleration_structure_build_range_infos.push_back(accelerationBuildRangeInfo);

        primitive_counts.push_back(geometry.primitiveCount);
        geometry.updated = false;
    }

    VkAccelerationStructureBuildGeometryInfoKHR build_geometry_info{};
    build_geometry_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    build_geometry_info.type = type;
    build_geometry_info.flags = flags;
    build_geometry_info.mode = mode;
    if (mode == VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR && handle != VK_NULL_HANDLE) {
        build_geometry_info.srcAccelerationStructure = handle;
        build_geometry_info.dstAccelerationStructure = handle;
    }
    build_geometry_info.geometryCount = static_cast<uint32_t>(acceleration_structure_geometries.size());
    build_geometry_info.pGeometries = acceleration_structure_geometries.data();

    VkAccelerationStructureBuildSizesInfoKHR build_sizes_info{};
    build_sizes_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    GetAccelerationStructureBuildSizesKHR(
        device,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &build_geometry_info,
        &geometries[0].primitiveCount,
        &build_sizes_info);

    if (buffer.handle == VK_NULL_HANDLE || buffer.size != build_sizes_info.accelerationStructureSize) {
        buffer = ressource_builder.createBuffer(
                build_sizes_info.accelerationStructureSize,
                VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                );

        VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
        accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        accelerationStructureCreateInfo.buffer = buffer.handle;
        accelerationStructureCreateInfo.size = build_sizes_info.accelerationStructureSize;
        accelerationStructureCreateInfo.type = type;
        if (CreateAccelerationStructureKHR(device, &accelerationStructureCreateInfo, nullptr, &handle) != VK_SUCCESS) {
            throw std::runtime_error("failed to create bl acceleration structure!");
        }
    }


    AllocatedBuffer scratchBuffer = ressource_builder.createBuffer(
        build_sizes_info.buildScratchSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    build_geometry_info.dstAccelerationStructure = handle;
    build_geometry_info.scratchData.deviceAddress = scratchBuffer.deviceAddress;

    VkCommandBuffer cmdBuffer = command_manager.beginSingleTimeCommands();
    auto as_build_range_infos = &*acceleration_structure_build_range_infos.data();
    CmdBuildAccelerationStructuresKHR(
        device,
        cmdBuffer,
        1,
        &build_geometry_info,
        &as_build_range_infos
        );
    command_manager.endSingleTimeCommand(cmdBuffer);

    ressource_builder.destroyBuffer(scratchBuffer);

    VkAccelerationStructureDeviceAddressInfoKHR accelerationStructureDeviceAddressInfo{};
    accelerationStructureDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    accelerationStructureDeviceAddressInfo.accelerationStructure = handle;
    device_address = GetAccelerationStructureDeviceAddressKHR(device, &accelerationStructureDeviceAddressInfo);
}

void AccelerationStructure::destroy() {
    if (buffer.handle != VK_NULL_HANDLE) {
        ressource_builder.destroyBuffer(buffer);
    }

    if (instance_buffer.handle != VK_NULL_HANDLE) {
        ressource_builder.destroyBuffer(instance_buffer);
    }

    if (handle != VK_NULL_HANDLE) {
        DestroyAccelerationStructureKHR(device, handle, nullptr);
    }
}

VkAccelerationStructureKHR AccelerationStructure::getHandle() const {
    return handle;
}

uint64_t AccelerationStructure::getDeviceAddress() const {
    return device_address;
}