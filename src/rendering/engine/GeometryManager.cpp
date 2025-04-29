#include <GeometryManager.hpp>
#include <MeshRenderer.hpp>

namespace RtEngine {

void collect_mesh_assets_recursive(const std::shared_ptr<Node>& root_node, std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<MeshAsset>>>& mesh_map)
{
    for (auto child_node : root_node->children)
    {
        std::shared_ptr<MeshRenderer> mesh_renderer = child_node->getComponent<MeshRenderer>();
        if (mesh_renderer && !mesh_map->contains(mesh_renderer->meshAsset->name))
        {
            (*mesh_map)[mesh_renderer->meshAsset->name] = mesh_renderer->meshAsset;
        }
        collect_mesh_assets_recursive(child_node, mesh_map);
    }
}

std::vector<std::shared_ptr<MeshAsset>> collect_mesh_assets(const std::shared_ptr<Node>& root_node)
{
    auto mesh_map = std::make_shared<std::unordered_map<std::string, std::shared_ptr<MeshAsset>>>();
    collect_mesh_assets_recursive(root_node, mesh_map);

    std::vector<std::shared_ptr<MeshAsset>> mesh_assets;
    for (auto mesh_asset : *mesh_map)
    {
        mesh_assets.push_back(mesh_asset.second);
    }
    return mesh_assets;
}

void GeometryManager::createGeometryBuffers(const std::shared_ptr<Node>& root_node)
{
    std::vector<std::shared_ptr<MeshAsset>> mesh_assets = collect_mesh_assets(root_node);
    vertex_buffer = createVertexBuffer(mesh_assets);
    index_buffer = createIndexBuffer(mesh_assets);
    geometry_mapping_buffer = createGeometryMappingBuffer(mesh_assets);
}


AllocatedBuffer GeometryManager::createVertexBuffer(std::vector<std::shared_ptr<MeshAsset>>& mesh_assets) const
{
    assert(!mesh_assets.empty());

    if (vertex_buffer.handle != VK_NULL_HANDLE)
    {
        resource_builder->destroyBuffer(vertex_buffer);
    }

    VkDeviceSize size =  0;
    for (auto& mesh_asset : mesh_assets) {
        size += mesh_asset->meshBuffers.vertices.size();
    }

    std::vector<Vertex> vertices(size);

    uint32_t vertex_offset = 0;
    for (auto& mesh_asset : mesh_assets) {
        vertices.insert(vertices.begin() + vertex_offset, mesh_asset->meshBuffers.vertices.begin(), mesh_asset->meshBuffers.vertices.end());

        mesh_asset->instance_data.vertex_offset = vertex_offset;
        vertex_offset += mesh_asset->meshBuffers.vertices.size();
    }

    return resource_builder->stageMemoryToNewBuffer(vertices.data(), vertices.size() * sizeof(Vertex), VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
        | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

AllocatedBuffer GeometryManager::createIndexBuffer(std::vector<std::shared_ptr<MeshAsset>>& mesh_assets) const
{
    assert(!mesh_assets.empty());

    if (index_buffer.handle != VK_NULL_HANDLE)
    {
        resource_builder->destroyBuffer(index_buffer);
    }

    VkDeviceSize size =  0;
    for (auto& mesh_asset : mesh_assets) {
        size += mesh_asset->meshBuffers.indices.size();
    }

    std::vector<uint32_t> indices(size);

    uint32_t index_offset = 0;
    for (auto& mesh_asset : mesh_assets) {
        indices.insert(indices.begin() + index_offset, mesh_asset->meshBuffers.indices.begin(), mesh_asset->meshBuffers.indices.end());

        mesh_asset->instance_data.triangle_offset = index_offset;
        index_offset += mesh_asset->meshBuffers.indices.size();
    }

    return resource_builder->stageMemoryToNewBuffer(indices.data(), indices.size() * sizeof(uint32_t), VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
        | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

AllocatedBuffer GeometryManager::createGeometryMappingBuffer(std::vector<std::shared_ptr<MeshAsset>>& mesh_assets) const
{
    assert(!mesh_assets.empty());

    if (geometry_mapping_buffer.handle != VK_NULL_HANDLE)
    {
        resource_builder->destroyBuffer(geometry_mapping_buffer);
    }

    std::vector<GeometryData> geometry_datas;
    for (auto& mesh_asset : mesh_assets) {
        geometry_datas.push_back(mesh_asset->instance_data);
    }
    return resource_builder->stageMemoryToNewBuffer(geometry_datas.data(), mesh_assets.size() * sizeof(GeometryData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}


AllocatedBuffer GeometryManager::getVertexBuffer() const
{
    assert(vertex_buffer.handle != VK_NULL_HANDLE);
    return vertex_buffer;
}

AllocatedBuffer GeometryManager::getIndexBuffer() const
{
    assert(index_buffer.handle != VK_NULL_HANDLE);
    return index_buffer;
}

AllocatedBuffer GeometryManager::getGeometryMappingBuffer() const
{
    assert(geometry_mapping_buffer.handle != VK_NULL_HANDLE);
    return geometry_mapping_buffer;
}

void GeometryManager::destroy()
{
    if (vertex_buffer.handle != VK_NULL_HANDLE)
        resource_builder->destroyBuffer(vertex_buffer);
    if (index_buffer.handle != VK_NULL_HANDLE)
        resource_builder->destroyBuffer(index_buffer);
    if (geometry_mapping_buffer.handle != VK_NULL_HANDLE)
        resource_builder->destroyBuffer(geometry_mapping_buffer);
}

}
