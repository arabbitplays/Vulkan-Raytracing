//
// Created by oschdi on 3/19/25.
//

#include "MeshRenderer.hpp"
#include <Node.hpp>

MeshRenderer::MeshRenderer(std::shared_ptr<Node> node) : Component(node) {}

void MeshRenderer::OnRender(DrawContext& ctx)
{
    auto shared_node = node.lock();
    if (!shared_node) {
        assert(false);
    }

    glm::mat4 nodeMatrix = shared_node->transform->getWorldTransform();

    ctx.objects.push_back(RenderObject{InstanceData{meshAsset->geometry_id, meshMaterial->material_index}, meshAsset->accelerationStructure, nodeMatrix, meshAsset->triangle_count});
}