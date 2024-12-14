//
// Created by oschdi on 12/14/24.
//

#include "BottomLevelAccelerationStructureBuilder.hpp"

void BottomLevelAccelerationStructureBuilder::addTriangleGeometry(const MeshAsset &mesh) {
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

    Geometry geometry{};
    geometry.handle = accelerationStructureGeometry;
    geometry.primitiveCount = mesh.surfaces[0].count / 3;// TODO verallgemeinern
    geometries.push_back(geometry);
}