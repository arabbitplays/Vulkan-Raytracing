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
    CommandManager& command_manager) {
    this->device = device;
    this->ressource_builder = ressource_builder;
    this->command_manager = command_manager;
}

AccelerationStructure AccelerationStructureBuilder::buildAccelerationStructure(VkAccelerationStructureTypeKHR type,
        VkBuildAccelerationStructureFlagsKHR flags, VkBuildAccelerationStructureModeKHR mode) {
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
    if (mode == VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR && acceleration_structure.handle != VK_NULL_HANDLE) {
        build_geometry_info.srcAccelerationStructure = acceleration_structure.handle;
        build_geometry_info.dstAccelerationStructure = acceleration_structure.handle;
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

    if (acceleration_structure.buffer.handle == VK_NULL_HANDLE) {

    }
    acceleration_structure.buffer = ressource_builder.createBuffer(
        build_sizes_info.accelerationStructureSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
    accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelerationStructureCreateInfo.buffer = acceleration_structure.buffer.handle;
    accelerationStructureCreateInfo.size = build_sizes_info.accelerationStructureSize;
    accelerationStructureCreateInfo.type = type;
    if (CreateAccelerationStructureKHR(device, &accelerationStructureCreateInfo, nullptr, &acceleration_structure.handle) != VK_SUCCESS) {
        throw std::runtime_error("failed to create bl acceleration structure!");
    }

    AllocatedBuffer scratchBuffer = ressource_builder.createBuffer(
        build_sizes_info.buildScratchSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
    accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationBuildGeometryInfo.type = type;
    accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    accelerationBuildGeometryInfo.dstAccelerationStructure = acceleration_structure.handle;
    accelerationBuildGeometryInfo.geometryCount = static_cast<uint32_t>(geometries.size());
    accelerationBuildGeometryInfo.pGeometries = &geometries[0].handle; //
    accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

    VkCommandBuffer cmdBuffer = command_manager.beginSingleTimeCommands();
    auto as_build_range_infos = &*acceleration_structure_build_range_infos.data();
    CmdBuildAccelerationStructuresKHR(
        device,
        cmdBuffer,
        1,
        &accelerationBuildGeometryInfo,
        &as_build_range_infos
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

