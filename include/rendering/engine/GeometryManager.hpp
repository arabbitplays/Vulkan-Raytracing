#ifndef GEOMETRYMANAGER_HPP
#define GEOMETRYMANAGER_HPP

#include <VulkanContext.hpp>
#include <Scene.hpp>

namespace RtEngine {

class GeometryManager {
public:
    GeometryManager() = default;
    GeometryManager(const std::shared_ptr<ResourceBuilder>& resource_builder) : resource_builder(resource_builder) {}

    void createGeometryBuffers(const std::shared_ptr<Node>& root_node);

    AllocatedBuffer getVertexBuffer() const;
    AllocatedBuffer getIndexBuffer() const;
    AllocatedBuffer getGeometryMappingBuffer() const;

    void destroy();
private:
    AllocatedBuffer createVertexBuffer(std::vector<std::shared_ptr<MeshAsset>>& mesh_assets) const;
    AllocatedBuffer createIndexBuffer(std::vector<std::shared_ptr<MeshAsset>>& mesh_assets) const;
    AllocatedBuffer createGeometryMappingBuffer(std::vector<std::shared_ptr<MeshAsset>>& mesh_assets) const;

    std::shared_ptr<ResourceBuilder> resource_builder;
    AllocatedBuffer vertex_buffer, index_buffer, geometry_mapping_buffer;
};
}

#endif //GEOMETRYMANAGER_HPP