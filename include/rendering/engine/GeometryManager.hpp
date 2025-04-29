#ifndef GEOMETRYMANAGER_HPP
#define GEOMETRYMANAGER_HPP

#include <VulkanContext.hpp>
#include <Scene.hpp>

namespace RtEngine {

struct GeometryBuffers {
    AllocatedBuffer vertex_buffer, index_buffer, geometry_mapping_buffer, instance_mapping_buffer, emitting_instances_buffer;
    uint32_t emitting_instances_count;
};

class GeometryManager {
public:
    GeometryManager() = default;
    GeometryManager(const std::shared_ptr<ResourceBuilder>& resource_builder) : resource_builder(resource_builder) {}

    std::shared_ptr<GeometryBuffers> createGeometryBuffers(std::shared_ptr<Node> root_node);
private:
    AllocatedBuffer createVertexBuffer(std::vector<std::shared_ptr<MeshAsset>>& mesh_assets);
    AllocatedBuffer createIndexBuffer(std::vector<std::shared_ptr<MeshAsset>>& mesh_assets);
    AllocatedBuffer createGeometryMappingBuffer(std::vector<std::shared_ptr<MeshAsset>>& mesh_assets);
    AllocatedBuffer createInstanceMappingBuffer(std::vector<RenderObject> &objects);
    AllocatedBuffer createEmittingInstancesBuffer(std::vector<RenderObject> &objects, std::shared_ptr<Material> material, uint32_t* emitting_instances_count);
    AllocatedBuffer getEmittingInstancesBuffer();

    std::shared_ptr<ResourceBuilder> resource_builder;
};
}

#endif //GEOMETRYMANAGER_HPP