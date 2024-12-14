//
// Created by oschdi on 12/14/24.
//

#include "AccelerationStructureBuilder.hpp"

#include <DeletionQueue.hpp>

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

AccelerationStructureBuilder::AccelerationStructureBuilder(VkDevice& device, RessourceBuilder& ressource_builder,
    CommandManager& command_manager, DeletionQueue& deletion_queue) {
    this->device = device;
    this->ressource_builder = ressource_builder;
    this->command_manager = command_manager;
    this->deletion_queue = deletion_queue;
}

AccelerationStructure AccelerationStructureBuilder::buildBlasFromMesh(const MeshAsset &mesh) {
    VkDeviceOrHostAddressConstKHR vertexDeviceAddress{};
    VkDeviceOrHostAddressConstKHR indexDeviceAddress{};
    vertexDeviceAddress.deviceAddress = mesh.meshBuffers.vertexBuffer.deviceAddress;
    indexDeviceAddress.deviceAddress = mesh.meshBuffers.indexBuffer.deviceAddress;

    VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
    accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    accelerationStructureGeometry.geometry.triangles.vertexData = vertexDeviceAddress;
    accelerationStructureGeometry.geometry.triangles.maxVertex = mesh.surfaces[0].count;
    accelerationStructureGeometry.geometry.triangles.vertexStride = sizeof(Vertex);
    accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
    accelerationStructureGeometry.geometry.triangles.indexData = indexDeviceAddress;

    uint32_t primitiveCount = mesh.surfaces[0].count / 3; // TODO verallgemeinern
    std::vector<VkAccelerationStructureGeometryKHR> geometries{accelerationStructureGeometry};
    return buildAccelerationStructure(VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, primitiveCount, geometries);
}

AccelerationStructure AccelerationStructureBuilder::buildTlasFromMesh(std::vector<AccelerationStructure>& blas) {

    VkTransformMatrixKHR transform_matrix = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f};

    VkAccelerationStructureInstanceKHR accelerationStructureInstance{};
    accelerationStructureInstance.transform = transform_matrix;
    accelerationStructureInstance.instanceCustomIndex = 0;
    accelerationStructureInstance.mask = 0xFF;
    accelerationStructureInstance.instanceShaderBindingTableRecordOffset = 0;
    accelerationStructureInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    accelerationStructureInstance.accelerationStructureReference = blas[0].deviceAddress; // TODO verallgemeinern

    AllocatedBuffer instanceBuffer = ressource_builder.createBuffer(
        sizeof(VkAccelerationStructureInstanceKHR),
        VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    instanceBuffer.update(device, &accelerationStructureInstance, sizeof(VkAccelerationStructureInstanceKHR));

    VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
    instanceDataDeviceAddress.deviceAddress = instanceBuffer.deviceAddress;

    VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
    accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
    accelerationStructureGeometry.geometry.instances.data = instanceDataDeviceAddress;

    std::vector<VkAccelerationStructureGeometryKHR> geometries{accelerationStructureGeometry};
    AccelerationStructure result = buildAccelerationStructure(VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, 1, geometries);

    ressource_builder.destroyBuffer(instanceBuffer);
    return result;
}

AccelerationStructure AccelerationStructureBuilder::buildAccelerationStructure(VkAccelerationStructureTypeKHR type, uint32_t primitiveCount,
    std::vector<VkAccelerationStructureGeometryKHR>& geometries) {

    AccelerationStructure acceleration_structure{};

    VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
    accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationStructureBuildGeometryInfo.type = type;
    accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationStructureBuildGeometryInfo.geometryCount = static_cast<uint32_t>(geometries.size());
    accelerationStructureBuildGeometryInfo.pGeometries = geometries.data();

    VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
    accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    GetAccelerationStructureBuildSizesKHR(
        device,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &accelerationStructureBuildGeometryInfo,
        &primitiveCount,
        &accelerationStructureBuildSizesInfo);

    acceleration_structure.buffer = ressource_builder.createBuffer(
        accelerationStructureBuildSizesInfo.accelerationStructureSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
    accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelerationStructureCreateInfo.buffer = acceleration_structure.buffer.handle;
    accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
    accelerationStructureCreateInfo.type = type;
    if (CreateAccelerationStructureKHR(device, &accelerationStructureCreateInfo, nullptr, &acceleration_structure.handle) != VK_SUCCESS) {
        throw std::runtime_error("failed to create bl acceleration structure!");
    }

    AllocatedBuffer scratchBuffer = ressource_builder.createBuffer(
        accelerationStructureBuildSizesInfo.buildScratchSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
    accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationBuildGeometryInfo.type = type;
    accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    accelerationBuildGeometryInfo.dstAccelerationStructure = acceleration_structure.handle;
    accelerationBuildGeometryInfo.geometryCount = static_cast<uint32_t>(geometries.size());
    accelerationBuildGeometryInfo.pGeometries = geometries.data();
    accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

    VkAccelerationStructureBuildRangeInfoKHR accelerationBuildRangeInfo{};
    accelerationBuildRangeInfo.primitiveCount = primitiveCount;
    accelerationBuildRangeInfo.primitiveOffset = 0;
    accelerationBuildRangeInfo.firstVertex = 0;
    accelerationBuildRangeInfo.transformOffset = 0;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildRangeInfos = {&accelerationBuildRangeInfo};

    VkCommandBuffer cmdBuffer = command_manager.beginSingleTimeCommands();
    CmdBuildAccelerationStructuresKHR(
        device,
        cmdBuffer,
        1,
        &accelerationBuildGeometryInfo,
        accelerationBuildRangeInfos.data()
        );
    command_manager.endSingleTimeCommand(cmdBuffer);

    ressource_builder.destroyBuffer(scratchBuffer);

    VkAccelerationStructureDeviceAddressInfoKHR accelerationStructureDeviceAddressInfo{};
    accelerationStructureDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    accelerationStructureDeviceAddressInfo.accelerationStructure = acceleration_structure.handle;
    acceleration_structure.deviceAddress = GetAccelerationStructureDeviceAddressKHR(device, &accelerationStructureDeviceAddressInfo);

    return acceleration_structure;
}

void AccelerationStructureBuilder::destroyAccelerationStructure(AccelerationStructure& accelerationStructure) {
    if (accelerationStructure.handle) {
        ressource_builder.destroyBuffer(accelerationStructure.buffer);
        DestroyAccelerationStructureKHR(device, accelerationStructure.handle, nullptr);
    }
}

