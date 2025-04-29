#include <GeometryManager.hpp>
#include <MeshRenderer.hpp>

namespace RtEngine {

void collect_mesh_assets_recursive(std::shared_ptr<Node> root_node, std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<MeshAsset>>>& mesh_map)
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

std::vector<std::shared_ptr<MeshAsset>> collect_mesh_assets(std::shared_ptr<Node> root_node)
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

std::shared_ptr<GeometryBuffers> GeometryManager::createGeometryBuffers(std::shared_ptr<Node> root_node)
{
    std::vector<std::shared_ptr<MeshAsset>> mesh_assets = collect_mesh_assets(root_node);
    return nullptr;
}


AllocatedBuffer GeometryManager::createVertexBuffer(std::vector<std::shared_ptr<MeshAsset>>& mesh_assets) {
    assert(!mesh_assets.empty());

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

AllocatedBuffer GeometryManager::createIndexBuffer(std::vector<std::shared_ptr<MeshAsset>>& mesh_assets) {
    assert(!mesh_assets.empty());

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

AllocatedBuffer GeometryManager::createGeometryMappingBuffer(std::vector<std::shared_ptr<MeshAsset>>& mesh_assets) {
    assert(!mesh_assets.empty());

    std::vector<GeometryData> geometry_datas;
    for (auto& mesh_asset : mesh_assets) {
        geometry_datas.push_back(mesh_asset->instance_data);
    }
    return resource_builder->stageMemoryToNewBuffer(geometry_datas.data(), mesh_assets.size() * sizeof(GeometryData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

AllocatedBuffer GeometryManager::createInstanceMappingBuffer(std::vector<RenderObject>& objects) {
    assert(!objects.empty());

    std::vector<InstanceData> instance_datas;
    for (int i = 0; i < objects.size(); i++) {
        instance_datas.push_back(objects[i].instance_data);
    }

    return resource_builder->stageMemoryToNewBuffer(instance_datas.data(), instance_datas.size() * sizeof(InstanceData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}

AllocatedBuffer GeometryManager::createEmittingInstancesBuffer(std::vector<RenderObject>& objects, std::shared_ptr<Material> material, uint32_t* emitting_instances_count) {
    assert(!objects.empty());

    std::vector<EmittingInstanceData> emitting_instances;
    for (int i = 0; i < objects.size(); i++) {
        EmittingInstanceData instance_data;
        instance_data.instance_id = i;
        instance_data.model_matrix = objects[i].transform;
        float power = material->getEmissionForInstance(objects[i].instance_data.material_index).w;
        instance_data.primitive_count = objects[i].primitive_count;
        if (power > 0.0f || (i == objects.size() - 1 && emitting_instances.empty())) {
            emitting_instances.push_back(instance_data);
        }
    }

    *emitting_instances_count = emitting_instances.size();
    return resource_builder->stageMemoryToNewBuffer(emitting_instances.data(), emitting_instances.size() * sizeof(EmittingInstanceData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
}
}
